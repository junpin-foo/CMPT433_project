#ifndef _GPS_H
#define _GPS_H

#include <stdbool.h>

struct location {
    double latitude;
    double longitude;
    double speed;
};

#define INVALID_LATITUDE -1000
#define INVALID_LONGITUDE -1000
#define INVALID_SPEED -1
void GPS_init();
void GPS_cleanup();
void GPS_demoInit(); 
bool GPS_hasSignal();
struct location GPS_getLocation();

#endif