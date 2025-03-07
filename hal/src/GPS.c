#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int serial_port;

void GPS_init() { 
    serial_port = open("/dev/ttyAMA0", O_RDWR); 
    if (serial_port < 0) { 
        printf("Error %i from open: %s\n", errno, strerror(errno)); 
     }

    struct termios tty;

    if(tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    }

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag |= ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    tty.c_cc[VTIME] = 10; // deciseconds
    tty.c_cc[VMIN] = 0; // deciseconds

    cfsetspeed(&tty, B9600);
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }
}


char* GPS_read() {
    static char read_buf[255];
    while (1) {  // Keep reading until we find a $GNGGA message
        int n = read(serial_port, read_buf, sizeof(read_buf) - 1); // Leave space for null terminator

        if (n > 0) {
            read_buf[n] = '\0'; // Properly terminate the string
            
            // Check if the received message starts with "$GNGGA"
            if (strncmp(read_buf, "$GNGGA", 6) == 0) {
                // printf("Geo graphic data received: %s\n", read_buf);
                return read_buf;
            }
        }
    }
}

void parse_GNGGA(char* gngga_sentence, double* latitude, double* longitude) {
    
    char *token;
    char temp[255];
    
    // Set latitude and longitude to invalid values
    *latitude = -1000;
    *longitude = -1000;

    // Make a copy to avoid modifying the original
    strcpy(temp, gngga_sentence);
    
    // Skip the $GNGGA
    token = strtok(temp, ",");
    if (token == NULL || strcmp(token, "$GNGGA") != 0) {
        return; // Invalid sentence (not starting with $GNGGA)
    }

    // Skip time (ignore)
    token = strtok(NULL, ",");
    if (token == NULL) return; // Invalid sentence

    // Latitude
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return; // Invalid sentence (missing latitude)

    double raw_lat = atof(token);
    int lat_deg = (int)(raw_lat / 100); // Extract degrees
    double lat_min = raw_lat - (lat_deg * 100); // Extract minutes
    *latitude = lat_deg + (lat_min / 60.0); // Convert to decimal degrees

    // N/S Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'S') *latitude = -*latitude;

    // Longitude
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return; // Invalid sentence (missing longitude)

    double raw_lon = atof(token);
    int lon_deg = (int)(raw_lon / 100); // Extract degrees
    double lon_min = raw_lon - (lon_deg * 100); // Extract minutes
    *longitude = lon_deg + (lon_min / 60.0); // Convert to decimal degrees

    // E/W Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'W') *longitude = -*longitude;

    // Fix quality check: if quality is 0, it's invalid
    token = strtok(NULL, ",");
    if (token && atoi(token) == 0) {
        *latitude = -1000;
        *longitude = -1000;
    }
}