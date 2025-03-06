// speed_limit.h
#ifndef SPEED_LIMIT_H
#define SPEED_LIMIT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Function to get estimated speed limit based on GPS coordinates
int get_speed_limit(double latitude, double longitude);

#endif