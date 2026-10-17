#include <stdio.h>
#include "cooling_controller.h"

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

    int temperatures[] = {99, 100, 109, 110, 106, 105, 96, 95};

    int count = sizeof(temperatures) / sizeof(temperatures[0]);

    printf("AUTOMOTIVE ENGINE COOLING SIMULATION\n");
    printf("------------------------------------\n");

    for (int i = 0; i < count; i++) {

        cooling_update(&controller, temperatures[i]);

        printf("Temperature: %d C | Fan: %s\n",
               temperatures[i],
               cooling_state_name(controller.state));
    }

    return 0;
}