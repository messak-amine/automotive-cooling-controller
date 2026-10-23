
#include <stdio.h>
#include "cooling_controller.h"

typedef struct {
    int temperature;
    bool sensor_valid;
} SensorSample;

int main(void)
{
    CoolingConfig config = {
        .low_on_c = 100,
        .low_off_c = 95,
        .high_on_c = 110,
        .high_off_c = 105
    };

    CoolingController controller;
    cooling_init(&controller, config);

    SensorSample samples[] = {
        {99, true},
        {100, true},
        {110, true},
        {106, true},
        {105, true},
        {95, true},
        {101, false},
        {92, true},
        {91, true},
        {90, true},
        {200, true},
        {96, true},
        {94, true},
        {93, true}
    };

    int count = sizeof(samples) / sizeof(samples[0]);

    printf("AUTOMOTIVE ENGINE COOLING SIMULATION\n");
    printf("------------------------------------\n");

    for (int i = 0; i < count; i++) {
        cooling_update(&controller,
                       samples[i].temperature,
                       samples[i].sensor_valid);

        printf("Temp: %3d C | Input: %-7s | Fan: %-4s | Fault: %s\n",
               samples[i].temperature,
               samples[i].sensor_valid ? "VALID" : "INVALID",
               cooling_state_name(controller.state),
               controller.sensor_fault ? "YES" : "NO");
    }

    return 0;
}
