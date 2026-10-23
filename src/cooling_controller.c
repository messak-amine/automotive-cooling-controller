
#include "cooling_controller.h"

#define SENSOR_MIN_C (-40)
#define SENSOR_MAX_C 150
#define RECOVERY_SAMPLES 3U

void cooling_init(CoolingController *controller, CoolingConfig config)
{
    controller->state = FAN_OFF;
    controller->config = config;
    controller->sensor_fault = false;
    controller->valid_streak = 0;
}

void cooling_update(CoolingController *controller,
                    int temperature,
                    bool sensor_valid)
{
    bool reading_ok = sensor_valid &&
                      temperature >= SENSOR_MIN_C &&
                      temperature <= SENSOR_MAX_C;

    // Invalid sensor reading: activate fail-safe cooling
    if (!reading_ok) {
        controller->sensor_fault = true;
        controller->valid_streak = 0;
        controller->state = FAN_HIGH;
        return;
    }

    // Require three consecutive valid readings to recover
    if (controller->sensor_fault) {
        controller->valid_streak++;

        if (controller->valid_streak < RECOVERY_SAMPLES) {
            controller->state = FAN_HIGH;
            return;
        }

        controller->sensor_fault = false;
        controller->valid_streak = 0;
    }

    // Normal cooling control
    switch (controller->state) {
        case FAN_OFF:
            if (temperature >= controller->config.high_on_c) {
                controller->state = FAN_HIGH;
            }
            else if (temperature >= controller->config.low_on_c) {
                controller->state = FAN_LOW;
            }
            break;

        case FAN_LOW:
            if (temperature >= controller->config.high_on_c) {
                controller->state = FAN_HIGH;
            }
            else if (temperature <= controller->config.low_off_c) {
                controller->state = FAN_OFF;
            }
            break;

        case FAN_HIGH:
            if (temperature <= controller->config.high_off_c) {
                if (temperature <= controller->config.low_off_c) {
                    controller->state = FAN_OFF;
                }
                else {
                    controller->state = FAN_LOW;
                }
            }
            break;

        default:
            controller->state = FAN_HIGH;
            break;
    }
}

const char *cooling_state_name(FanState state)
{
    switch (state) {
        case FAN_OFF:  return "OFF";
        case FAN_LOW:  return "LOW";
        case FAN_HIGH: return "HIGH";
        default:       return "UNKNOWN";
    }
}
