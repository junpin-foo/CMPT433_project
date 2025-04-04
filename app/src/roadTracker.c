#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sys/wait.h>
#include <assert.h>
#include "hal/GPS.h"
#include "streetAPI.h"
#include "sleep_and_timer.h"
#include "roadTracker.h"

#define EARTH_RADIUS 6371.0 // Radius of Earth in kilometers
#define M_PI 3.14159265358979323846

static pthread_t roadTrackerThread;
static bool isRunning = false;
static bool isInitialized = false;

static struct location target_location = {INVALID_LATITUDE, INVALID_LONGITUDE, -1};
static struct location souruce_location = {INVALID_LATITUDE, INVALID_LONGITUDE, -1};
static struct location current_location = {INVALID_LATITUDE, INVALID_LONGITUDE, -1};
static bool target_set = false;
static void* trackLocationThreadFunc(void* arg);
static double totalDistanceNeeded = -1;
static double current_distance = -1;
static double progress = 0;
static char target_address[256] = "";

// Convert degrees to radians
static double deg_to_rad(double deg) {
    return deg * (M_PI / 180.0);
}

void runCommand(const char* command) {
    if (system(command) == -1) {
        printf("%s\n", command);
        printf("error: system() command failed\n");
    } else {
        printf("%s\n", command);
    }
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
            current_location  = GPS_getLocation();
            if (current_location.latitude == INVALID_LATITUDE) {
                progress = 0;
                printf("Unable to get GPS data, GPS no Signal !\n");
            } else {
                current_location  = GPS_getLocation();
                printf("Current Location: Latitude %.6f, Longitude: %.6f, Speed: %.6f \n", current_location.latitude, current_location.longitude, current_location.speed);
                current_distance = haversine_distance(current_location, target_location);
                if (totalDistanceNeeded > 0) {
                    progress = ((totalDistanceNeeded - current_distance) / totalDistanceNeeded) * 100;
                    printf("Progress: %.2f%%\n", progress);
                    if (progress < 0) {
                        progress = 0;
                    } else if (current_distance <= 0.2) { // If within 200 meters of target
                        progress = 100;
                        sleepForMs(2000);
                        target_set = false;
                    }
                }
            }
        }
        sleepForMs(200);
        // For testing
        // target_set = true;
        // progress = 0;
        // sleepForMs(3000);
        // progress = 25;
        // sleepForMs(3000);
        // progress = 50;
        // sleepForMs(3000);
        // progress = 75;
        // sleepForMs(3000);
        // progress = 100;
        // sleepForMs(10000);
        // target_set = false;
        // progress = 0;
        // sleepForMs(3000);
    }
    return NULL;
}
// Expecting to be call from microphone
// Function to set the target location
// It will play the audio to let user know the target location is set or not
static void resetLocationInfo() {
    target_location.latitude = INVALID_LATITUDE;
    target_location.longitude = INVALID_LONGITUDE;
    souruce_location.latitude = INVALID_LATITUDE;
    souruce_location.longitude = INVALID_LONGITUDE;
    current_location.latitude = INVALID_LATITUDE;
    current_location.longitude = INVALID_LONGITUDE;
    totalDistanceNeeded = -1;
    current_distance = -1;
    progress = 0;
}

bool RoadTracker_setTarget(char *address) {
    assert(isInitialized);
    target_location = StreetAPI_get_lat_long(address);
    printf("Target Location: Latitude %.6f, Longitude %.6f\n", target_location.latitude, target_location.longitude);
    if (target_location.latitude == INVALID_LATITUDE) {
        strcpy(target_address, "");
        runCommand("espeak 'Fail to set the Target Location due to invalid input address. Check the input address again ' -w invalidTargetError.wav");
        runCommand("aplay -q invalidTargetError.wav");
        // printf("Fail to set the Target Location due to invalid address. Check the address again !\n");
        resetLocationInfo();
        return false;
    }
    souruce_location  = GPS_getLocation();
    if (souruce_location.latitude == INVALID_LATITUDE) {
        strcpy(target_address, "");
        runCommand("espeak 'Fail to set the Target Location due to invalid current location. Check the GPS signal again ' -w invalidCurrentError.wav");
        runCommand("aplay -q invalidCurrentError.wav");
        // printf("Fail to set the Target Location due to invalid current location. Check the GPS signal again !\n");
        resetLocationInfo();
        return false;
    } else {
        strncpy(target_address, address, sizeof(target_address) - 1);
        target_address[sizeof(target_address) - 1] = '\0'; // Ensure null termination
        totalDistanceNeeded = haversine_distance(souruce_location, target_location);
        runCommand("espeak 'Successfully setting the target destination ' -w successSet.wav");
        runCommand("aplay -q successSet.wav");
        target_set = true;
        printf("Target set to: Latitude %.6f, Longitude %.6f | Source Location: Latitude %.6f, Longitude %.6f | Total Distance: %.2f km\n", target_location.latitude, target_location.longitude, souruce_location.latitude, souruce_location.longitude, totalDistanceNeeded);
        return true;
    }
}


struct location RoadTracker_getSourceLocation(void) {
    return souruce_location;
}

struct location RoadTracker_getCurrentLocation(void) {
    return souruce_location;
}

// Function to get the target location
struct location RoadTracker_getTargetLocation(void) {
    return target_location;
}

// Function to get the target location address
char* RoadTracker_getTargetAddress(void) {
    return target_address;
}

// Function to get progress percentage
double RoadTracker_getProgress(void) {
    return progress;
}

// Function to get progress done
bool RoadTracker_isRunning(void) {
    return target_set;
}