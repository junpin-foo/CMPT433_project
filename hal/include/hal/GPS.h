/* GPS.h
*  This file is part of the GPS module it provides an interface to interact with GPS hardware.
*   It returns a location structure containing latitude, longitude, and speed.
*/
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

// Calling this will enable a thread read the gps data from demo_gps.txt. See "demo_locationData.txt" in project folder for more info"
// The cmake command already added the demo_gps.txt file to the /mnt/remote folder
void GPS_demoInit(); 


bool GPS_hasSignal();

// This function will return a location structure containing latitude, longitude, and speed.
struct location GPS_getLocation();

#endif