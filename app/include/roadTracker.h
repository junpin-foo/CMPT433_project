// roadtracker.h
#ifndef ROADTRACKER_H
#define ROADTRACKER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "streetAPI.h"

double haversine_distance(struct location loc1, struct location loc2);
void RoadTracker_setTarget(double latitude, double longitude);
void RoadTracker_init();
void RoadTracker_cleanup();

#endif // ROADTRACKER_H