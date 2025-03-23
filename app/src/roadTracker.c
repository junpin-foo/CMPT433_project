#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include "hal/GPS.h"
#include "sleep_and_timer.h"

#define EARTH_RADIUS 6371.0 // Radius of Earth in kilometers
#define M_PI 3.14159265358979323846

static pthread_t roadTrackerThread;
static bool isRunning = false;
static bool isInitialized = false;

struct location {
    double latitude;
    double longitude;
};

static struct location target_location = {0.0, 0.0};
static bool target_set = true;
static void* trackLocationThreadFunc(void* arg);

// Convert degrees to radians
static double deg_to_rad(double deg) {
    return deg * (M_PI / 180.0);
}

// Haversine formula to calculate distance between two locations
static double haversine_distance(struct location loc1, struct location loc2) {
    double dlat = deg_to_rad(loc2.latitude - loc1.latitude);
    double dlon = deg_to_rad(loc2.longitude - loc1.longitude);

    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(deg_to_rad(loc1.latitude)) * cos(deg_to_rad(loc2.latitude)) *
               sin(dlon / 2) * sin(dlon / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS * c; // Distance in kilometers
}

// Initialization function
void RoadTracker_init(void) {
    assert(!isInitialized);
    isRunning = true;
    isInitialized = true;
    pthread_create(&roadTrackerThread, NULL, &trackLocationThreadFunc, NULL);
}

// Cleanup function
void RoadTracker_cleanup(void) {
    assert(isInitialized);
    isRunning = false;
    pthread_join(roadTrackerThread, NULL);
    isInitialized = false;
}


// Thread function to track location
double current_distance = -1;
static void* trackLocationThreadFunc(void* arg) {
    assert(isInitialized);
    (void)arg; // Suppress unused parameter warning
    while (isRunning) {
        if (target_set) {
            char* gps_data = GPS_read();
            struct location current_location = {0.0, 0.0};
            double speed = 0;
            parse_GPRMC(gps_data, &current_location.latitude, &current_location.longitude, &speed);

            if (current_location.latitude != -1000 && current_location.longitude != -1000) {
                current_distance = haversine_distance(current_location, target_location);
                printf("Distance to target: %.2f km\n", current_distance);
            } else {
                printf("Invalid GPS data.\n");
            }
        }
        sleepForMs(1000);
    }
    return NULL;
}

// Function to set the target location
void RoadTracker_setTarget(double latitude, double longitude) {
    assert(isInitialized);
    target_location.latitude = latitude;
    target_location.longitude = longitude;
    target_set = true;
    printf("Target set to: Latitude %.6f, Longitude %.6f\n", latitude, longitude);
}
