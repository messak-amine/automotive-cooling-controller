#ifndef COOLING_CONTROLLER_H
#define COOLING_CONTROLLER_H

typedef enum {
    FAN_OFF,
    FAN_LOW,
    FAN_HIGH
} FanState;

typedef struct {
    int low_on_c;
    int low_off_c;
    int high_on_c;
    int high_off_c;
} CoolingConfig;

typedef struct {
    FanState state;
    CoolingConfig config;
} CoolingController;

void cooling_init(CoolingController *controller, CoolingConfig config);

void cooling_update(CoolingController *controller, int temperature);

const char *cooling_state_name(FanState state);

#endif