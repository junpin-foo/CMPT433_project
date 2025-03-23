// speed_limit.h
#ifndef STREET_H
#define STREET_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct location {
    double latitude;
    double longitude;
};
struct location get_lat_long(const char *address);

#endif