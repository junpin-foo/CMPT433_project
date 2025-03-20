#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sleep_and_timer.h"
#include "hal/accelerometer.h"
#include "hal/i2c.h"
#include "hal/GPS.h"
#include "speedLed.h"

int main() {
    Ic2_initialize();
    // Initialize accelerometer and GPS
    Accelerometer_initialize();
    // GPS_init();
    SpeedLED_init();

    while(1){
        sleepForMs(1000);
    }

    // Cleanup resources
    Ic2_cleanUp();
    Accelerometer_cleanUp();
    return 0;
}
