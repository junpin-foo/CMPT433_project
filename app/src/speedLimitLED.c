
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include "speedLimitAPI.h"
#include "hal/GPS.h"
#include "hal/accelerometer.h"
#include "sleep_and_timer.h"
#include <assert.h>
#include <math.h>

static pthread_t updateLEDThread;
static pthread_t updateSpeedLimitThread;
static bool isRunning = false;
static bool isInitialized = false;

#define SAMPLING_PERIOD_MS 100  // 100ms sampling period
#define SAMPLING_PERIOD_SEC 0.1 // Converted to seconds
#define GRAVITY 9.81            // Earth's gravity in m/s²
#define ACCEL_THRESHOLD 0.7    // Ignore noise below this threshold (was 0.15)
#define DECAY_FACTOR 0.7       // Simulate friction and air resistance (was 0.95)

float car_speed = 0.0;  // Initial speed in m/s
double speed_kmh = 0.0; 
int speedLimit = 0;

static float getHorizontalAcceleration(AccelerometerData data) {
    // Convert from G to m/s²
    float ax = data.x * GRAVITY;
    float ay = data.y * GRAVITY;

    // Compute horizontal acceleration magnitude
    float accel_horizontal = sqrt(ax * ax + ay * ay);

    // Ignore small vibrations (noise filter)
    return (accel_horizontal >= ACCEL_THRESHOLD) ? accel_horizontal : 0;
}

static void* updateSpeedAndLEDThreadFunc(void* arg) {
    (void)arg; // Suppress unused parameter warning
    while (isRunning) {

        //get coords from GPS
        //use GPS and accelerometer to calculate speed
        //get speed limit from GPS and API
        //compare speed to speed limit -> set LED

        // Get accelerometer reading
        AccelerometerData data = Accelerometer_getReading();
        float accel_horizontal = getHorizontalAcceleration(data);

        // Compute speed change using v = u + at
        float speed_change = accel_horizontal * SAMPLING_PERIOD_SEC;
        
        // Update speed
        if (accel_horizontal > 0) {
            car_speed += speed_change; // Accelerate
        } else {
            car_speed *= DECAY_FACTOR; // Apply friction
        }

        // Convert to km/h
        float acc_speed_kmh = car_speed * 3.6;

        // Get GPS reading 
        struct location current_location = GPS_getLocation();
        double gps_speed_kmh = current_location.speed;

        speed_kmh = acc_speed_kmh; //set as accelerometer data

        // Use GPS speed if available
        // if (gps_speed > 0) {
        //     speed_kmh = gps_speed;
        // }

        
        if (speed_kmh > speedLimit) {
            // Over speed limit -> LED Red
        } else if (speed_kmh - speedLimit > 10) {
            // 10km/h before over -> LED Yellow
        } else {
            // Within limit -> Green
        }
        
        // Print values
        printf("X: %f, Y: %f, Z: %f\n", data.x, data.y, data.z);
        printf("Acceleration: %.2f m/s²\n", accel_horizontal);
        printf("Speed Change: %.2f m/s\n", speed_change);
        printf("Current Speed: (gps) %.2f km/h, (acc) %.2f km/h\n", gps_speed_kmh, acc_speed_kmh);

        sleepForMs(SAMPLING_PERIOD_MS);
        
    }
    return NULL;
}

static void* updateSpeedLimitFunc(void* arg) {
    (void)arg; // Suppress unused parameter warning
    while (isRunning) {
        // Get GPS reading 
        struct location current_location  = {-1000, -1000, -1};
        // struct location current_location = GPS_getLocation();
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