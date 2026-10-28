#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "cooling_controller.h"

typedef struct {
    uint8_t last_pwm_duty;
    bool last_fault_led;
    unsigned int pwm_call_count;
    unsigned int fault_call_count;
} HalSpy;

static void spy_set_pwm(uint8_t duty_pct, void *user_ctx) {
    HalSpy *spy = (HalSpy *)user_ctx;
    if (spy != NULL) {
        spy->last_pwm_duty = duty_pct;
        spy->pwm_call_count++;
    }
}

static void spy_set_fault_led(bool fault_active, void *user_ctx) {
    HalSpy *spy = (HalSpy *)user_ctx;
    if (spy != NULL) {
        spy->last_fault_led = fault_active;
        spy->fault_call_count++;
    }
}

static CoolingController make_controller(void) {
    CoolingConfig config = {
        .low_on_c = 100,
        .low_off_c = 95,
        .high_on_c = 110,
        .high_off_c = 105
    };
    CoolingController controller;
    CoolingStatus status = cooling_init(&controller, &config);
    assert(status == COOLING_OK);
    return controller;
}

static void test_config_validation(void) {
    CoolingController controller;
    CoolingConfig valid = { .low_on_c = 100, .low_off_c = 95, .high_on_c = 110, .high_off_c = 105 };
    CoolingConfig inverted = { .low_on_c = 95, .low_off_c = 100, .high_on_c = 110, .high_off_c = 105 };
    CoolingConfig overlap = { .low_on_c = 106, .low_off_c = 95, .high_on_c = 110, .high_off_c = 105 };
    CoolingConfig out_of_range = { .low_on_c = 100, .low_off_c = -45, .high_on_c = 110, .high_off_c = 105 };

    assert(cooling_init(NULL, &valid) == COOLING_ERR_NULL);
    assert(cooling_init(&controller, NULL) == COOLING_ERR_NULL);
    assert(cooling_init(&controller, &inverted) == COOLING_ERR_CONFIG);
    assert(cooling_init(&controller, &overlap) == COOLING_ERR_CONFIG);
    assert(cooling_init(&controller, &out_of_range) == COOLING_ERR_CONFIG);
    assert(cooling_init(&controller, &valid) == COOLING_OK);
}

static void test_normal_operation(void) {
    CoolingController c = make_controller();

    assert(cooling_update(&c, 90, true) == FAN_OFF);
    assert(cooling_update(&c, 100, true) == FAN_LOW);
    assert(cooling_update(&c, 110, true) == FAN_HIGH);
}

static void test_low_hysteresis(void) {
    CoolingController c = make_controller();

    assert(cooling_update(&c, 100, true) == FAN_LOW);
    assert(cooling_update(&c, 98, true) == FAN_LOW);
    assert(cooling_update(&c, 96, true) == FAN_LOW);
    assert(cooling_update(&c, 95, true) == FAN_OFF);
}

static void test_high_hysteresis(void) {
    CoolingController c = make_controller();

    assert(cooling_update(&c, 110, true) == FAN_HIGH);
    assert(cooling_update(&c, 108, true) == FAN_HIGH);
    assert(cooling_update(&c, 106, true) == FAN_HIGH);
    assert(cooling_update(&c, 105, true) == FAN_LOW);
    assert(cooling_update(&c, 95, true) == FAN_OFF);
}

static void test_direct_high_activation(void) {
    CoolingController c = make_controller();

    assert(cooling_update(&c, 85, true) == FAN_OFF);
    assert(cooling_update(&c, 115, true) == FAN_HIGH);
}

static void test_sensor_failure(void) {
    CoolingController c = make_controller();

    assert(cooling_update(&c, 85, false) == FAN_HIGH);
    assert(c.sensor_fault == true);
}

static void test_invalid_temperature(void) {
    CoolingController c = make_controller();

    assert(cooling_update(&c, -41, true) == FAN_HIGH);
    assert(c.sensor_fault == true);

    c = make_controller();
    assert(cooling_update(&c, 151, true) == FAN_HIGH);
    assert(c.sensor_fault == true);
}

static void test_fault_recovery(void) {
    /* Recovery to OFF when T <= low_off_c (95 C) */
    CoolingController c = make_controller();
    assert(cooling_update(&c, 90, false) == FAN_HIGH);
    assert(c.sensor_fault == true);
    assert(cooling_update(&c, 90, true) == FAN_HIGH);
    assert(cooling_update(&c, 90, true) == FAN_HIGH);
    assert(cooling_update(&c, 90, true) == FAN_OFF);
    assert(c.sensor_fault == false);

    /* Recovery to LOW when low_off_c < T <= high_off_c (95 C < T <= 105 C) */
    c = make_controller();
    assert(cooling_update(&c, 102, false) == FAN_HIGH);
    assert(cooling_update(&c, 102, true) == FAN_HIGH);
    assert(cooling_update(&c, 102, true) == FAN_HIGH);
    assert(cooling_update(&c, 102, true) == FAN_LOW);
    assert(c.sensor_fault == false);

    /* Recovery in upper hysteresis band (T > 105 C) clears fault but stays HIGH */
    c = make_controller();
    assert(cooling_update(&c, 108, false) == FAN_HIGH);
    assert(cooling_update(&c, 108, true) == FAN_HIGH);
    assert(cooling_update(&c, 108, true) == FAN_HIGH);
    assert(cooling_update(&c, 108, true) == FAN_HIGH);
    assert(c.sensor_fault == false);
}

static void test_interrupted_recovery(void) {
    CoolingController c = make_controller();

    assert(cooling_update(&c, 90, false) == FAN_HIGH);
    assert(cooling_update(&c, 90, true) == FAN_HIGH);
    assert(cooling_update(&c, 90, true) == FAN_HIGH);

    assert(cooling_update(&c, 200, true) == FAN_HIGH);
    assert(c.valid_streak == 0U);

    assert(cooling_update(&c, 90, true) == FAN_HIGH);
    assert(cooling_update(&c, 90, true) == FAN_HIGH);
    assert(cooling_update(&c, 90, true) == FAN_OFF);
    assert(c.sensor_fault == false);
}

static void test_state_getters_and_hal(void) {
    CoolingController c = make_controller();
    HalSpy spy = { 0 };
    CoolingHal hal = {
        .set_fan_pwm = spy_set_pwm,
        .set_fault_led = spy_set_fault_led,
        .user_ctx = &spy
    };

    assert(cooling_set_hal(&c, &hal) == COOLING_OK);
    assert(cooling_get_state(&c) == FAN_OFF);
    assert(cooling_get_pwm_duty(&c) == 0U);
    assert(strcmp(cooling_state_name(FAN_OFF), "OFF") == 0);
    assert(strcmp(cooling_state_name(FAN_LOW), "LOW") == 0);
    assert(strcmp(cooling_state_name(FAN_HIGH), "HIGH") == 0);

    cooling_update(&c, 102, true);
    assert(cooling_get_state(&c) == FAN_LOW);
    assert(spy.last_pwm_duty == 50U);
    assert(spy.last_fault_led == false);

    cooling_update(&c, 90, false);
    assert(cooling_get_state(&c) == FAN_HIGH);
    assert(spy.last_pwm_duty == 100U);
    assert(spy.last_fault_led == true);
}

int main(void) {
    test_config_validation();
    test_normal_operation();
    test_low_hysteresis();
    test_high_hysteresis();
    test_direct_high_activation();
    test_sensor_failure();
    test_invalid_temperature();
    test_fault_recovery();
    test_interrupted_recovery();
    test_state_getters_and_hal();

    printf("All cooling controller unit tests passed (10/10).\n");
    return 0;
}
