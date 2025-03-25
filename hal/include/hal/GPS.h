#ifndef _GPS_H
#define _GPS_H

struct location {
    double latitude;
    double longitude;
    double speed;
};

#define INVALID_LATITUDE -1000
#define INVALID_LONGITUDE -1000
#define INVALID_SPEED -1
void GPS_init();
struct location GPS_getLocation();

#endif