/* 
 * This header file defines the interface for the StreetAPI module,
 * which provides functions for converting between human-readable addresses
 * and geographical coordinates (latitude and longitude) using the 
 * OpenStreetMap Nominatim API.
 */

#ifndef STREET_H
#define STREET_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Structure to store the response data from a CURL request.
struct Response {
    char *data;
    size_t size;
};
// Initializes and clean up the StreetAPI module.
void StreetAPI_init();
void StreetAPI_cleanup();

// Retrieves latitude and longitude coordinates for a given address string.
struct location StreetAPI_get_lat_long(char *address);

// Retrieves a address for the given latitude and longitude.
char* StreetAPI_get_address_from_lat_lon(double lat, double lon);
#endif