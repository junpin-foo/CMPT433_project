#ifndef _GPS_H
#define _GPS_H

struct location {
    double latitude;
    double longitude;
};

void GPS_init();
char* GPS_read();
void parse_GPRMC(char* gngga_sentence, double* latitude, double* longitude, double* speed);

#endif