
#include <assert.h>
#include <stdio.h>
#include "cooling_controller.h"

static CoolingController make_controller(void)
{
    CoolingConfig config = {
        .low_on_c = 100,
        .low_off_c = 95,
        .high_on_c = 110,
        .high_off_c = 105
    };

    CoolingController controller;
    cooling_init(&controller, config);
    return controller;
}

static void test_normal_operation(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 99, true);
    assert(controller.state == FAN_OFF);

    cooling_update(&controller, 100, true);
    assert(controller.state == FAN_LOW);

    cooling_update(&controller, 110, true);
    assert(controller.state == FAN_HIGH);

    assert(controller.sensor_fault == false);
}

static void test_low_hysteresis(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 100, true);
    assert(controller.state == FAN_LOW);

    cooling_update(&controller, 96, true);
    assert(controller.state == FAN_LOW);

    cooling_update(&controller, 95, true);
    assert(controller.state == FAN_OFF);
}

static void test_high_hysteresis(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 110, true);
    assert(controller.state == FAN_HIGH);

    cooling_update(&controller, 106, true);
    assert(controller.state == FAN_HIGH);

    cooling_update(&controller, 105, true);
    assert(controller.state == FAN_LOW);
}

static void test_direct_high_activation(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 120, true);

    assert(controller.state == FAN_HIGH);
    assert(controller.sensor_fault == false);
}

static void test_sensor_failure(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 90, false);

    assert(controller.sensor_fault == true);
    assert(controller.state == FAN_HIGH);
    assert(controller.valid_streak == 0);
}

static void test_invalid_temperature(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 200, true);

    assert(controller.sensor_fault == true);
    assert(controller.state == FAN_HIGH);
}

static void test_fault_recovery(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 90, false);

    cooling_update(&controller, 90, true);
    assert(controller.sensor_fault == true);
    assert(controller.state == FAN_HIGH);

    cooling_update(&controller, 90, true);
    assert(controller.sensor_fault == true);
    assert(controller.state == FAN_HIGH);

    cooling_update(&controller, 90, true);
    assert(controller.sensor_fault == false);
    assert(controller.state == FAN_OFF);
}

static void test_interrupted_recovery(void)
{
    CoolingController controller = make_controller();

    cooling_update(&controller, 90, false);

    cooling_update(&controller, 90, true);
    cooling_update(&controller, 90, true);
    assert(controller.valid_streak == 2);

    cooling_update(&controller, 90, false);
    assert(controller.valid_streak == 0);
    assert(controller.state == FAN_HIGH);

    cooling_update(&controller, 90, true);
    assert(controller.valid_streak == 1);
    assert(controller.sensor_fault == true);
}

int main(void)
{
    test_normal_operation();
    puts("[PASS] Normal operation");

    test_low_hysteresis();
    puts("[PASS] Low-speed hysteresis");

    test_high_hysteresis();
    puts("[PASS] High-speed hysteresis");

    test_direct_high_activation();
    puts("[PASS] Direct high-speed activation");

    test_sensor_failure();
    puts("[PASS] Sensor failure");

    test_invalid_temperature();
    puts("[PASS] Invalid temperature");

    test_fault_recovery();
    puts("[PASS] Fault recovery");

    test_interrupted_recovery();
    puts("[PASS] Interrupted recovery");

    puts("\nAll 8 tests passed.");
    return 0;
}
