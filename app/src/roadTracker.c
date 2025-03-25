#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include "hal/GPS.h"
#include "streetAPI.h"
#include "sleep_and_timer.h"

#define EARTH_RADIUS 6371.0 // Radius of Earth in kilometers
#define M_PI 3.14159265358979323846

static pthread_t roadTrackerThread;
static bool isRunning = false;
static bool isInitialized = false;

static struct location target_location = {0.0, 0.0, -1};
static bool target_set = false;
static void* trackLocationThreadFunc(void* arg);
static double totalDistanceNeeded = -1;
static double current_distance = -1;
static double progress = 0;

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
static void* trackLocationThreadFunc(void* arg) {
    assert(isInitialized);
    (void)arg; // Suppress unused parameter warning
    while (isRunning) {
        if (target_set) {
            struct location current_location = {49.255280, -122.811226, -1};
            current_distance = haversine_distance(current_location, target_location);
            if (totalDistanceNeeded > 0) {
                progress = ((totalDistanceNeeded - current_distance) / totalDistanceNeeded) * 100;
                if (current_distance <= 0.05) { // If within 50 meters of target
                    printf("Target reached. Resetting target.\n");
                    target_set = false;
                    totalDistanceNeeded = -1;
                    target_location.latitude = 0.0;
                    target_location.longitude = 0.0;
                }
                printf("Distance to target: %.2f km | Progress: %.2f%%\n", current_distance, progress);
            } else {
                printf("Distance to target: %.2f km\n", current_distance);
            }
        }
        sleepForMs(1000);
    }
    return NULL;
}

// Expecting to be call from microphone
// Function to set the target location
void RoadTracker_setTarget(char *address) {
    assert(isInitialized);
    struct location current_location  = GPS_getLocation();
    // struct location current_location = {49.255280, -122.811226, -1};
    if (current_location.latitude == -INVALID_LATITUDE) {
        printf("Unable to set up target due to invalid current location\n");
    } else {
        target_location = StreetAPI_get_lat_long("Simon Fraser University");
        totalDistanceNeeded = haversine_distance(current_location, target_location);
        target_set = true;
        printf("Target set to: Latitude %.6f, Longitude %.6f | Total Distance: %.2f km\n", target_location.latitude, target_location.longitude, totalDistanceNeeded);
    }
}

