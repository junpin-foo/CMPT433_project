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

void configure_serial(int serial_port) {
    struct termios options;

    // Get current settings
    tcgetattr(serial_port, &options);

    // Set timeout behavior
    options.c_cc[VMIN]  = 0;   // Minimum number of characters to read
    options.c_cc[VTIME] = 10;  // Timeout in tenths of a second (10 = 1 second)

    // Apply settings
    tcsetattr(serial_port, TCSANOW, &options);
}


char* GPS_read() {
    static char read_buf[255];
    configure_serial(serial_port);
    while (1) {  // Keep reading until we find a $GNGGA message
        printf("waiting for GPS DATA\n");
        int n = read(serial_port, &read_buf, sizeof(read_buf)); // Leave space for null terminator
        if (n > 0) {
            read_buf[n] = '\0'; // Properly terminate the string
            
            // Check if the received message starts with "$GNGGA"
            if (strncmp(read_buf, "$GNGGA", 6) == 0) {
                // printf("Geo graphic data received: %s\n", read_buf);
                return read_buf;
            }
        }
        sleep(1);
    }
}

void parse_GPRMC(char* gprmc_sentence, double* latitude, double* longitude, double* speed) {
    char *token;
    char temp[255];

    // Set default invalid values
    *latitude = -1000;
    *longitude = -1000;
    *speed = -1;

    // Make a copy to avoid modifying the original
    strcpy(temp, gprmc_sentence);
    
    // Skip the $GPRMC
    token = strtok(temp, ",");
    if (token == NULL || strcmp(token, "$GPRMC") != 0) {
        return; // Invalid sentence (not starting with $GPRMC)
    }
    
    // Skip time (ignore)
    token = strtok(NULL, ",");
    if (token == NULL) return;
    
    // Status (A = valid, V = void)
    token = strtok(NULL, ",");
    if (token == NULL || token[0] != 'A') {
        return; // Invalid sentence (data not valid)
    }

    // Latitude
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return;
    double raw_lat = atof(token);
    int lat_deg = (int)(raw_lat / 100);
    double lat_min = raw_lat - (lat_deg * 100);
    *latitude = lat_deg + (lat_min / 60.0);

    // N/S Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'S') *latitude = -*latitude;

    // Longitude
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return;
    double raw_lon = atof(token);
    int lon_deg = (int)(raw_lon / 100);
    double lon_min = raw_lon - (lon_deg * 100);
    *longitude = lon_deg + (lon_min / 60.0);

    // E/W Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'W') *longitude = -*longitude;

    // Speed in knots
    token = strtok(NULL, ",");
    if (token != NULL && strlen(token) > 0) {
        *speed = atof(token) * 1.852;
    }
}
