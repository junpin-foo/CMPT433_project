#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sleep_and_timer.h"
#include "hal/accelerometer.h"
#include "hal/i2c.h"
#include "hal/GPS.h"
#include "streetAPI.h"
#include "speedLimitLED.h"
#include "roadTracker.h"
#include "updateLcd.h"

int main() {
    Ic2_initialize();
    // Initialize accelerometer and GPS
    Accelerometer_initialize();
    GPS_init();
    UpdateLcd_init();
    SpeedLED_init();
    StreetAPI_init();
    RoadTracker_init();
    struct location test = StreetAPI_get_lat_long("Simon Fraser University");
    RoadTracker_setTarget(test.latitude, test.longitude);
    while(1){
    // char address[256];
        // printf("Enter address: ");
        // fgets(address, sizeof(address), stdin);
        // address[strcspn(address, "\n")] = 0;  // Remove newline character
        // struct location test = StreetAPI_get_lat_long(address);
        // printf("%f\n", test.latitude);
        // printf("%f\n", test.longitude);
        sleepForMs(1000);
    }
    // Cleanup resources
    RoadTracker_cleanup();
    StreetAPI_cleanup();
    Ic2_cleanUp();
    Accelerometer_cleanUp();
    return 0;
}

