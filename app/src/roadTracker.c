/*
* This file implements the RoadTracker module, which tracks the progress of a target location
* using GPS data. It provides functions to set a target location, get the current location,
* calculate the distance to the target, and reset the target location. Check the header file for more details.
**/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sys/wait.h>
#include <assert.h>
#include <ctype.h>
#include "hal/GPS.h"
#include "streetAPI.h"
#include "sleep_and_timer.h"
#include "roadTracker.h"
#include <stdatomic.h>
#include "speedLimitLED.h"

#define EARTH_RADIUS 6371.0 // Radius of Earth in kilometers
#define M_PI 3.14159265358979323846
#define THRESHOLD_REACH 0.3
#define SLEEP_TIME_FOR_PROGRESS_FULL 5000

static pthread_t roadTrackerThread;
static bool isRunning = false;
static bool isInitialized = false;
static pthread_mutex_t roadTrackerMutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect road tracker data

static struct location target_location = {INVALID_LATITUDE, INVALID_LONGITUDE, -1};
static struct location souruce_location = {INVALID_LATITUDE, INVALID_LONGITUDE, -1};
static struct location current_location = {INVALID_LATITUDE, INVALID_LONGITUDE, -1};
static bool target_set = false;
static double totalDistanceNeeded = -1;
static double current_distance = -1;
static double progress = 0;
static char target_address[256] = "";

static void* trackLocationThreadFunc(void* arg);
static void runCommand(const char* command);
static void RoadTracker_resetData();
static double deg_to_rad(double deg);
static double haversine_distance(struct location loc1, struct location loc2);

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

// Thread function to track the progress of the target location
static void* trackLocationThreadFunc(void* arg) {
    assert(isInitialized);
    (void)arg;
    while (isRunning) {
        if (target_set) { // Only run if target is set
            current_location  = GPS_getLocation();
            if (current_location.latitude == INVALID_LATITUDE) {
                progress = 0; // Reset progress if GPS signal is invalid
                printf("Invalid Current Location. Check GPS signal !\n"); 
            } else {
                current_location  = GPS_getLocation();
                current_distance = haversine_distance(current_location, target_location);
                if (totalDistanceNeeded > 0) {
                    progress = ((totalDistanceNeeded - current_distance) / totalDistanceNeeded) * 100;
                    if (progress < 0) {
                        progress = 0;  // Prevent negative progress
                    } else if (current_distance <= THRESHOLD_REACH) { // Consider reach if within certain threshold to prevent the target is actually in the building
                        progress = 100;
                    }
                }
                if (progress == 100) {
                    // Sleep for 3 seconds before resetting target to display NeoPixel longer
                    printf("Target: Latitude %.6f, Longitude: %.6f, Current: Latitude %.6f, Longitude: %.6f, Speed: %.6f, Speed Limit: %d, progress: %.2f\n",target_location.latitude, target_location.longitude, current_location.latitude, current_location.longitude, current_location.speed, SpeedLED_getSpeedLimit(), progress);
                    sleepForMs(SLEEP_TIME_FOR_PROGRESS_FULL);
                    RoadTracker_resetData();
                } else {
                    printf("Target: Latitude %.6f, Longitude: %.6f, Current: Latitude %.6f, Longitude: %.6f, Speed: %.6f, Speed Limit: %d, progress: %.2f\n",target_location.latitude, target_location.longitude, current_location.latitude, current_location.longitude, current_location.speed, SpeedLED_getSpeedLimit(), progress);
                }
            }
        }
        sleepForMs(300);
    }
    return NULL;
}

// Function to reset the target location and data
void RoadTracker_resetTarget() {
    assert(isInitialized);
    pthread_mutex_lock(&roadTrackerMutex); // Lock the mutex before resetting target
    RoadTracker_resetData();
    runCommand("espeak -v mb-en1 -s 120  'Target location reset successfully' -w resetTarget.wav");
    runCommand("aplay -q resetTarget.wav");
    pthread_mutex_unlock(&roadTrackerMutex); // Unlock the mutex after resetting target
}

// Function to remove the trailing spaces from the address parsing from microphone
static void rtrim(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        len--;
    }
    str[len] = '\0';
}

// Expecting to be call from microphone
// Function to set the target location
// It will play the audio to let user know the target location is set or not
void RoadTracker_setTarget(char *address) {
    assert(isInitialized);
    pthread_mutex_lock(&roadTrackerMutex); // Lock the mutex before setting target
    souruce_location  = GPS_getLocation();
    rtrim(address);
    target_location = StreetAPI_get_lat_long(address);
    printf("Target Location: Latitude %.6f, Longitude %.6f\n", target_location.latitude, target_location.longitude);
    if (target_location.latitude == INVALID_LATITUDE) {
        // printf("Fail to set the Target Location due to invalid address. Check the address again !\n");
        runCommand("espeak -v mb-en1 -s 120  'Fail to set the Target Location due to invalid input address. Check the input address again ' -w invalidTargetError.wav");
        runCommand("aplay -q invalidTargetError.wav");
        RoadTracker_resetData();
    } else if (souruce_location.latitude == INVALID_LATITUDE) {
        // printf("Fail to set the Target Location due to invalid current location. Check the GPS signal again!\n");
        runCommand("espeak -v mb-en1 -s 120  'Fail to set the Target Location due to invalid current location. Check the GPS signal again ' -w invalidCurrentError.wav");
        runCommand("aplay -q invalidCurrentError.wav");
        RoadTracker_resetData();
    } else {
        // Set target Information
        totalDistanceNeeded = haversine_distance(souruce_location, target_location);
        strncpy(target_address, address, sizeof(target_address) - 1);
        target_address[sizeof(target_address) - 1] = '\0'; // Ensure null termination
        target_set = true;
        printf("Target set to: Latitude %.6f, Longitude %.6f | Source Location: Latitude %.6f, Longitude %.6f | Total Distance: %.2f km\n", target_location.latitude, target_location.longitude, souruce_location.latitude, souruce_location.longitude, totalDistanceNeeded);
        char sucessSetMsg[512]; // Adjust size if needed
        snprintf(sucessSetMsg, sizeof(sucessSetMsg), "espeak -v mb-en1 -s 120 'Successfully setting the target destination to %s' -w successSet.wav", target_address);
        runCommand(sucessSetMsg);
        runCommand("aplay -q successSet.wav");
    }
    pthread_mutex_unlock(&roadTrackerMutex); // Unlock the mutex after setting target
}

// Function to get the target location
static void RoadTracker_resetData() {
    target_set = false;
    target_location.latitude = INVALID_LATITUDE;
    target_location.longitude = INVALID_LONGITUDE;
    souruce_location.latitude = INVALID_LATITUDE;
    souruce_location.longitude = INVALID_LONGITUDE;
    current_location.latitude = INVALID_LATITUDE;
    current_location.longitude = INVALID_LONGITUDE;
    totalDistanceNeeded = -1;
    current_distance = -1;
    strcpy(target_address, "");
    progress = 0;
}

// Convert degrees to radians
static double deg_to_rad(double deg) {
    return deg * (M_PI / 180.0);
}

// Fuinction to run a system command
static void runCommand(const char* command) {
    if (system(command) == -1) {
        // printf("%s\n", command);
        printf("error: system() command failed\n");
    } else {
        // printf("%s\n", command);
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

// Function to get the current location
struct location RoadTracker_getCurrentLocation(void) {
    return souruce_location;
}

// Function to get the target location
struct location RoadTracker_getTargetLocation(void) {
    return target_location;
}


// Function to get progress percentage
double RoadTracker_getProgress(void) {
    return progress;
}

// Function to get progress done
bool RoadTracker_isRunning(void) {
    return target_set;
}