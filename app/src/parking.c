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
static bool reset = true;

static int prevColor = 2; // Assume green initially

// Hysteresis thresholds
static const float bad_enter_x = 0.15, bad_exit_x = 0.12;
static const float bad_enter_y = 0.15, bad_exit_y = 0.12;
static const float bad_enter_z_low = 0.95, bad_exit_z_low = 0.98;
static const float bad_enter_z_high = 1.05, bad_exit_z_high = 1.02;

static const float decent_enter_x = 0.07, decent_exit_x = 0.05;
static const float decent_enter_y = 0.07, decent_exit_y = 0.05;

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
        if(reset  && !RoadTracker_isRunning()) {
            reset = false;
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
        while (RoadTracker_isRunning() && isRunning) { //road tracker is running
            isParking = false;
            reset = true;
            mode = 0;
            sleepForMs(100);
            continue;
        }
        // printf("Parking activated\n");
        isParking = true;
        AccelerometerData data = Accelerometer_getReading();
        if (mode == 2) { // Flat surface detection mode
            bool bad = (fabs(data.x) > bad_enter_x || fabs(data.y) > bad_enter_y || data.z < bad_enter_z_low || data.z > bad_enter_z_high);
            bool bad_exit = (fabs(data.x) < bad_exit_x && fabs(data.y) < bad_exit_y && data.z > bad_exit_z_low && data.z < bad_exit_z_high);

            bool decent = (!bad) && (fabs(data.x) > decent_enter_x || fabs(data.y) > decent_enter_y);
            bool decent_exit = (fabs(data.x) < decent_exit_x && fabs(data.y) < decent_exit_y);

            if (prevColor == 0 && bad_exit) {
                prevColor = 1; // Move to yellow if exiting bad range
            } else if (bad) {
                prevColor = 0; // Stay in red
            } else if (prevColor == 1 && decent_exit) {
                prevColor = 2; // Move to green if exiting decent range
            } else if (decent) {
                prevColor = 1; // Stay in yellow
            } else {
                prevColor = 2; // Default to green
            }

            color = prevColor; // Apply updated color state
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
