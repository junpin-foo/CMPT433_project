// speed_limit.h
#ifndef STREET_H
#define STREET_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct Response {
    char *data;
    size_t size;
};
struct location StreetAPI_get_lat_long(char *address);
void StreetAPI_init();
void StreetAPI_cleanup();
#endif