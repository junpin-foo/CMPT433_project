
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include "speedLimitAPI.h"
#include "hal/GPS.h"
#include "sleep_and_timer.h"
#include <assert.h>
#include <math.h>

static pthread_t updateLEDThread;
static pthread_t updateSpeedLimitThread;
static bool isRunning = false;
static bool isInitialized = false;

#define SAMPLING_PERIOD_MS 100  // 100ms sampling period
double speed_kmh = 0.0; 
int speedLimit = 0;


static void* updateSpeedAndLEDThreadFunc(void* arg) {
    (void)arg; // Suppress unused parameter warning
    while (isRunning) {

        //get coords from GPS
        //use GPS and accelerometer to calculate speed
        //get speed limit from GPS and API
        //compare speed to speed limit -> set LED

        // Get GPS reading 
        struct location current_location = GPS_getLocation();
        double gps_speed_kmh = current_location.speed;
        speed_kmh = gps_speed_kmh;

        if (gps_speed_kmh > speedLimit) {
            // Over speed limit -> LED Red
        } else if (gps_speed_kmh - speedLimit > 10) {
            // 10km/h before over -> LED Yellow
        } else {
            // Within limit -> Green
        }
        
        printf("Current Speed: (gps) %.2f km/h\n", gps_speed_kmh);

        sleepForMs(SAMPLING_PERIOD_MS);
        
    }
    return NULL;
}

static void* updateSpeedLimitFunc(void* arg) {
    (void)arg; // Suppress unused parameter warning
    while (isRunning) {
        // Get GPS reading 
        // struct location current_location  = {-1000, -1000, -1};
        struct location current_location = GPS_getLocation();
        // double gps_speed_kmh = current_location.speed;

        speedLimit = get_speed_limit(current_location.latitude, current_location.longitude);
        printf("Speed Limit: %d km/h\n", speedLimit);
    
        sleepForMs(SAMPLING_PERIOD_MS);
        
    }
    return NULL;
}

void SpeedLED_init(void) {
    assert(!isInitialized);
    isRunning = true;
    isInitialized = true;
    pthread_create(&updateLEDThread, NULL, &updateSpeedAndLEDThreadFunc, NULL);
    pthread_create(&updateSpeedLimitThread, NULL, &updateSpeedLimitFunc, NULL);

}

void SpeedLED_cleanup(void) {
    assert(isInitialized);
    isRunning = false;
    pthread_join(updateLEDThread, NULL);
    pthread_join(updateSpeedLimitThread, NULL);
    isInitialized = false;
}

int SpeedLED_getSpeedLimit(void) {
    assert(isInitialized);
    return speedLimit;
}

double SpeedLED_getSpeed(void) {
    assert(isInitialized);
    return speed_kmh;
}