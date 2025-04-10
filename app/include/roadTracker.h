/* 
 * This header file defines the interface for the RoadTracker module,
 * It starts a background thread that calculates the distance between the current location and the target location, 
 * and computes a progress percentage from 0 to 100.
 * The module allows:
 *  - Setting a target location using a human-readable address.
 *  - Retrieving the current GPS location and the target location.
 *  - Tracking progress toward the target location in real time.
 * 
**/
#ifndef ROADTRACKER_H
#define ROADTRACKER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "streetAPI.h"

// Function to initialize and clean up the RoadTracker module
void RoadTracker_init();
void RoadTracker_cleanup();

// Fubnction to set the target location 
void RoadTracker_setTarget(char* address);

// Function to get the current location
struct location RoadTracker_getCurrentLocation();

// Function to get the target location
struct location RoadTracker_getTargetLocation();

// Function to get the target location address
char* RoadTracker_getTargetAddress();

// Function to get progress percentage
double RoadTracker_getProgress();

// Function to tell whether the road tracker is running or not
bool RoadTracker_isRunning();

// Function to clear the target location
void RoadTracker_resetTarget();

#endif // ROADTRACKER_H