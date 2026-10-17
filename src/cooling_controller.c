#include "cooling_controller.h"

void cooling_init(CoolingController *controller, CoolingConfig config)
{
    controller->state = FAN_OFF;
    controller->config = config;
}

void cooling_update(CoolingController *controller, int temperature)
{
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