// roadtracker.h
#ifndef ROADTRACKER_H
#define ROADTRACKER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "streetAPI.h"

void RoadTracker_init();
void RoadTracker_setTarget(char* address);
void RoadTracker_cleanup();

struct location RoadTracker_getSourceLocation();
// Function to get the target location
struct location RoadTracker_getTargetLocation();
// Function to get the target location address
char* RoadTracker_getTargetAddress();

// Function to get progress percentage
double RoadTracker_getProgress();

#endif // ROADTRACKER_H