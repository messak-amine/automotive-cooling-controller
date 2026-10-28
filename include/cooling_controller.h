#ifndef COOLING_CONTROLLER_H
#define COOLING_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#define COOLING_SENSOR_MIN_C (-40)
#define COOLING_SENSOR_MAX_C (150)
#define COOLING_RECOVERY_SAMPLES (3U)

typedef enum {
    FAN_OFF = 0,
    FAN_LOW,
    FAN_HIGH
} FanState;

typedef enum {
    COOLING_OK = 0,
    COOLING_ERR_NULL,
    COOLING_ERR_CONFIG
} CoolingStatus;

typedef struct {
    int16_t low_on_c;
    int16_t low_off_c;
    int16_t high_on_c;
    int16_t high_off_c;
} CoolingConfig;

typedef struct {
    void (*set_fan_pwm)(uint8_t duty_pct, void *user_ctx);
    void (*set_fault_led)(bool fault_active, void *user_ctx);
    void *user_ctx;
} CoolingHal;

typedef struct {
    CoolingConfig config;
    CoolingHal hal;
    FanState state;
    uint8_t pwm_duty_pct;
    bool sensor_fault;
    uint8_t valid_streak;
} CoolingController;

CoolingStatus cooling_init(CoolingController *controller, const CoolingConfig *config);
CoolingStatus cooling_set_hal(CoolingController *controller, const CoolingHal *hal);
FanState cooling_update(CoolingController *controller, int16_t temp_c, bool sensor_valid);
FanState cooling_get_state(const CoolingController *controller);
uint8_t cooling_get_pwm_duty(const CoolingController *controller);
const char *cooling_state_name(FanState state);

#endif
