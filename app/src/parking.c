#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include "hal/GPS.h"
#include "streetAPI.h"
#include "sleep_and_timer.h"
#include "roadTracker.h"
#include "hal/accelerometer.h"
#include "hal/joystick.h"
#include <unistd.h>
#include <math.h>
#include <stdatomic.h>

static pthread_t parkingThread;
static pthread_t modeThread;
static bool isRunning = false;
static bool isInitialized = false;
static bool isParking = false;
static atomic_int mode = 2; //1 for handbranke reminder, 2 for flat surface detection //0 for travel tracking
static atomic_int color = 0; //0 red bad, 1 yellow decent, 2 green good
static bool firstime = true;

//PROTOTYPE
static void* parkingThreadFunc(void* arg);
static void* modeThreadFunc(void* arg);

// Initialization function
void Parking_init(void) {
    assert(!isInitialized);
    isRunning = true;
    isInitialized = true;
    pthread_create(&modeThread, NULL, &modeThreadFunc, NULL);
    pthread_create(&parkingThread, NULL, &parkingThreadFunc, NULL);
}

// Cleanup function
void Parking_cleanup(void) {
    assert(isInitialized);
    isRunning = false;
    pthread_join(parkingThread, NULL);
    pthread_join(modeThread, NULL);
    isInitialized = false;
}

//Joystick thread
static void* modeThreadFunc(void* args) {
    (void) args;
    assert(isInitialized);
    while (isRunning) {
        JoystickDirection data = getJoystickDirection();

        struct location current_location = GPS_getLocation();
        double gps_speed_kmh = current_location.speed;
        if(gps_speed_kmh < 2 && firstime  && RoadTracker_isProgressDone()) {
            firstime = false;
            mode = 1; //default start handbreak reminder
        }

        if (data == JOYSTICK_UP) {
           mode = 1;
        } else if (data == JOYSTICK_DOWN) {
           mode = 2;
        }
        sleepForMs(100);
    }
    return NULL;
}

// Thread function
static void* parkingThreadFunc(void* arg) {
    assert(isInitialized);
    (void)arg; // Suppress unused parameter warning
    while (isRunning) {
        while (!RoadTracker_isProgressDone() && isRunning) { //road tracker not running
            isParking = false;
            firstime = true;
            mode = 0;
            sleepForMs(100);
            continue;
        }
        // printf("Parking activated\n");
        isParking = true;
        AccelerometerData data = Accelerometer_getReading();
        if (mode == 2) { // Flat surface detection mode
            bool bad = (fabs(data.x) > 0.2 || fabs(data.y) > 0.2 || data.z < 0.8 || data.z > 1.2);
            bool decent = (!bad) && (fabs(data.x) > 0.1 || fabs(data.y) > 0.1);

            //do calculation for angel
            if(bad) {
                color = 0; //red
            } else if(decent) {
                color = 1; //yellow
            }
            else {
                color = 2; //green
            }
        }

        sleepForMs(100); // Adjust the polling rate as needed
    }

    return NULL;
}

bool Parking_Activate(void) {
    return isParking;
}

int Parking_getMode(void) {
    return mode;
}

int Parking_getColor(void) {
    return color;
}
