
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include "api.h"
#include "hal/GPS.h"
#include "hal/accelerometer.h"
#include "sleep_and_timer.h"
#include <assert.h>
#include <math.h>

static pthread_t updateLEDThread;
static bool isRunning = false;
static bool isInitialized = false;

#define SAMPLING_PERIOD_MS 100  // 100ms sampling period
#define SAMPLING_PERIOD_SEC 0.1 // Converted to seconds
#define GRAVITY 9.81            // Earth's gravity in m/s²
#define ACCEL_THRESHOLD 0.7    // Ignore noise below this threshold (was 0.15)
#define DECAY_FACTOR 0.7       // Simulate friction and air resistance (was 0.95)

float car_speed = 0.0;  // Initial speed in m/s

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
    // double prev_latitude = 0.0, prev_longitude = 0.0;
    // int first_gps_reading = 1;
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
        float speed_kmh = car_speed * 3.6;

        // Get GPS reading 
        /*
        char* message = GPS_read();
        double latitude = 0.0, longitude = 0.0;
        parse_GNGGA(message, &latitude, &longitude);

        if (latitude != -1000 && longitude != -1000) {
            if (!first_gps_reading) {
                double gps_speed = calculate_gps_speed(prev_latitude, prev_longitude, latitude, longitude, SAMPLING_PERIOD_SEC);
                printf("GPS Speed: %.2f km/h\n", gps_speed);
                car_speed = (gps_speed / 3.6); // Sync estimated speed with GPS
            }
            prev_latitude = latitude;
            prev_longitude = longitude;
            first_gps_reading = 0;
        }
        */

        // Print values
        printf("X: %f, Y: %f, Z: %f\n", data.x, data.y, data.z);
        printf("Acceleration: %.2f m/s²\n", accel_horizontal);
        printf("Speed Change: %.2f m/s\n", speed_change);
        printf("Current Speed: %.2f m/s (%.2f km/h)\n", car_speed, speed_kmh);

        sleepForMs(SAMPLING_PERIOD_MS);
        
    }
    return NULL;
}

void SpeedLED_init(void) {
    assert(!isInitialized);
    isRunning = true;
    isInitialized = true;
    pthread_create(&updateLEDThread, NULL, &updateSpeedAndLEDThreadFunc, NULL);

}

void SpeedLED_cleanup(void) {
    assert(isInitialized);
    isRunning = false;
    pthread_join(updateLEDThread, NULL);
    isInitialized = false;
}