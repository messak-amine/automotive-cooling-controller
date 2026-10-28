#include "cooling_controller.h"
#include <stddef.h>

static uint8_t state_to_pwm(FanState state) {
    switch (state) {
        case FAN_LOW:
            return 50U;
        case FAN_HIGH:
            return 100U;
        case FAN_OFF:
        default:
            return 0U;
    }
}

static void apply_outputs(CoolingController *controller) {
    controller->pwm_duty_pct = state_to_pwm(controller->state);
    if (controller->hal.set_fan_pwm != NULL) {
        controller->hal.set_fan_pwm(controller->pwm_duty_pct, controller->hal.user_ctx);
    }
    if (controller->hal.set_fault_led != NULL) {
        controller->hal.set_fault_led(controller->sensor_fault, controller->hal.user_ctx);
    }
}

static bool is_config_valid(const CoolingConfig *cfg) {
    if (cfg == NULL) {
        return false;
    }
    if (cfg->low_off_c <= COOLING_SENSOR_MIN_C || cfg->high_on_c >= COOLING_SENSOR_MAX_C) {
        return false;
    }
    return (cfg->low_off_c < cfg->low_on_c) &&
           (cfg->low_on_c < cfg->high_off_c) &&
           (cfg->high_off_c < cfg->high_on_c);
}

static FanState evaluate_normal_state(FanState current, const CoolingConfig *cfg, int16_t temp_c) {
    switch (current) {
        case FAN_OFF:
            if (temp_c >= cfg->high_on_c) {
                return FAN_HIGH;
            }
            if (temp_c >= cfg->low_on_c) {
                return FAN_LOW;
            }
            return FAN_OFF;

        case FAN_LOW:
            if (temp_c >= cfg->high_on_c) {
                return FAN_HIGH;
            }
            if (temp_c <= cfg->low_off_c) {
                return FAN_OFF;
            }
            return FAN_LOW;

        case FAN_HIGH:
            if (temp_c <= cfg->low_off_c) {
                return FAN_OFF;
            }
            if (temp_c <= cfg->high_off_c) {
                return FAN_LOW;
            }
            return FAN_HIGH;

        default:
            return FAN_HIGH;
    }
}

CoolingStatus cooling_init(CoolingController *controller, const CoolingConfig *config) {
    if (controller == NULL || config == NULL) {
        return COOLING_ERR_NULL;
    }
    if (!is_config_valid(config)) {
        return COOLING_ERR_CONFIG;
    }

    controller->config = *config;
    controller->hal.set_fan_pwm = NULL;
    controller->hal.set_fault_led = NULL;
    controller->hal.user_ctx = NULL;
    controller->state = FAN_OFF;
    controller->pwm_duty_pct = 0U;
    controller->sensor_fault = false;
    controller->valid_streak = 0U;

    return COOLING_OK;
}

CoolingStatus cooling_set_hal(CoolingController *controller, const CoolingHal *hal) {
    if (controller == NULL || hal == NULL) {
        return COOLING_ERR_NULL;
    }
    controller->hal = *hal;
    apply_outputs(controller);
    return COOLING_OK;
}

FanState cooling_update(CoolingController *controller, int16_t temp_c, bool sensor_valid) {
    if (controller == NULL) {
        return FAN_HIGH;
    }

    bool in_range = (temp_c >= COOLING_SENSOR_MIN_C) && (temp_c <= COOLING_SENSOR_MAX_C);

    if (!sensor_valid || !in_range) {
        controller->sensor_fault = true;
        controller->valid_streak = 0U;
        controller->state = FAN_HIGH;
        apply_outputs(controller);
        return controller->state;
    }

    if (controller->sensor_fault) {
        controller->valid_streak++;
        if (controller->valid_streak < COOLING_RECOVERY_SAMPLES) {
            controller->state = FAN_HIGH;
            apply_outputs(controller);
            return controller->state;
        }
        controller->sensor_fault = false;
        controller->valid_streak = 0U;
    }

    controller->state = evaluate_normal_state(controller->state, &controller->config, temp_c);
    apply_outputs(controller);
    return controller->state;
}

FanState cooling_get_state(const CoolingController *controller) {
    if (controller == NULL) {
        return FAN_HIGH;
    }
    return controller->state;
}

uint8_t cooling_get_pwm_duty(const CoolingController *controller) {
    if (controller == NULL) {
        return 100U;
    }
    return controller->pwm_duty_pct;
}

const char *cooling_state_name(FanState state) {
    switch (state) {
        case FAN_OFF:
            return "OFF";
        case FAN_LOW:
            return "LOW";
        case FAN_HIGH:
            return "HIGH";
        default:
            return "UNKNOWN";
    }
}
