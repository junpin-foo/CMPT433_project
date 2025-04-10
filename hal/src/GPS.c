#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include "hal/GPS.h"
#include "stdbool.h"
#include "sleep_and_timer.h"

#define BUFFER_SIZE 255
#define DEGREE_FACTOR 100
#define MINUTES_IN_DEGREE 60.0
#define KNOTS_TO_KMH 1.852

int serial_port;
static pthread_t gps_thread;
static pthread_mutex_t gps_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to protect current_location
static struct location current_location = {INVALID_LATITUDE, INVALID_LONGITUDE, INVALID_SPEED};  // Default invalid location
static bool isRunning = false;
static bool signal = false;
static bool isInitialized = false;
static char* GPS_read();
static struct location parse_GNRMC(char* gprmc_sentence);

// Function that runs in the thread to continuously read GPS data
static void* gps_thread_func(void* arg) {
    (void)arg;
    assert(isInitialized);
    while (isRunning) {
        // Get the latest GPS data
        char* gps_data = GPS_read();
        // Parse the data into the location structure
        struct location new_location = parse_GNRMC(gps_data);

        // Update the global location safely using mutex
        pthread_mutex_lock(&gps_mutex);   // Lock the mutex before updating
        current_location = new_location;
        if (current_location.latitude == INVALID_LATITUDE) {
            signal = false;
            // printf("NO GPS Signal !\n");
        } else {
            // printf("Current_location: Latitude %.6f, Longitude: %.6f, Speed: %.6f \n", current_location.latitude, current_location.longitude, current_location.speed);
            signal = true;
        }
        pthread_mutex_unlock(&gps_mutex); // Unlock the mutex after updating
        
        sleepForMs(100);  // Sleep for 100ms before reading again
    }
    return NULL;
}

void GPS_init() {
    isRunning = true;
    serial_port = open("/dev/ttyAMA0", O_RDWR);
    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
        return;
    }

    struct termios tty;

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return;
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
        return;
    }
    isInitialized = true;
    // Create the GPS thread that will continuously read and update the location
    pthread_create(&gps_thread, NULL, gps_thread_func, NULL);
}

// Using for Demo purpose
static void* gps_thread2_func(void* arg) {
    (void)arg;
    while (isRunning) {
        pthread_mutex_lock(&gps_mutex);   // Lock the mutex before updating
        FILE* file = fopen("demo_gps.txt", "r");
        if (file == NULL) {
            perror("Failed to open file");
            return 0; // failure
        }

        if (fscanf(file, "%lf %lf %lf", &current_location.latitude, &current_location.longitude, &current_location.speed) != 3) {
            fprintf(stderr, "Invalid file format. Expected 3 numbers.\n");
            fclose(file);
            return 0; // failure
        }
        // printf("Demo: Current lat=%.6f, lon=%.6ff, spd=%.6f\n", current_location.latitude, current_location.longitude, current_location.speed);
        pthread_mutex_unlock(&gps_mutex); // Unlock the mutex after updating
        sleepForMs(500);
    }
    return NULL;
}

// For demo using to read geolocation data from demo_gps.txt
void GPS_demoInit() {
    signal = true;
    isRunning = true;
    pthread_create(&gps_thread, NULL, gps_thread2_func, NULL);
}


// Function to get the current GPS location
struct location GPS_getLocation() {
    struct location loc;
    pthread_mutex_lock(&gps_mutex);   // Lock the mutex before reading
    loc = current_location;
    pthread_mutex_unlock(&gps_mutex); // Unlock the mutex after reading
    return loc;  // Return the most recent GPS location
}

// Stati function to read GPS data from the serial port. This function only stop when finds a $GNRMC message (which include the location data and speed)
static char* GPS_read() {
    static char read_buf[BUFFER_SIZE];
    while (isRunning) {  // Keep reading until we find a $GNGGA message
        int n = read(serial_port, &read_buf, sizeof(read_buf)); // Leave space for null terminator
        if (n > 0) {
            read_buf[n] = '\0'; // Properly terminate the string
            // Check if the received message starts with "$GNRMC"
            if (strncmp(read_buf, "$GNRMC", 5) == 0) {
                return read_buf;
            }
        }
        sleepForMs(100);
    }
    return "";
}

// Function to parse the GNRMC sentence and extract latitude, longitude, and speed
static struct location parse_GNRMC(char* gnrmc_sentence) {
    char *token;
    char temp[BUFFER_SIZE];
    struct location data = {INVALID_LATITUDE, INVALID_LONGITUDE, INVALID_SPEED};
    struct location invalidData = {INVALID_LATITUDE, INVALID_LONGITUDE, INVALID_SPEED};

    // Make a copy to avoid modifying the original
    strcpy(temp, gnrmc_sentence);

    // Skip the $GNRMC identifier
    token = strtok(temp, ",");
    if (token == NULL || strcmp(token, "$GNRMC") != 0) {
        return invalidData; // Invalid sentence (not starting with $GNRMC)
    }
    
    // Skip time (ignore)
    token = strtok(NULL, ",");
    if (token == NULL) return invalidData;
    
    // Status (A = valid, V = void)
    token = strtok(NULL, ",");
    if (token == NULL || token[0] != 'A') {
        return invalidData; // Invalid sentence (data not valid)
    }

    // Latitude Calculation
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return invalidData;
    double raw_lat = atof(token);
    int lat_deg = (int)(raw_lat / DEGREE_FACTOR);
    double lat_min = raw_lat - (lat_deg * DEGREE_FACTOR);
    data.latitude = lat_deg + (lat_min / MINUTES_IN_DEGREE);

    // North/South Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'S') data.latitude = -data.latitude;

    // Longitude Calculation
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return invalidData;
    double raw_lon = atof(token);
    int lon_deg = (int)(raw_lon / DEGREE_FACTOR);
    double lon_min = raw_lon - (lon_deg * DEGREE_FACTOR);
    data.longitude = lon_deg + (lon_min / MINUTES_IN_DEGREE);

    // East/West Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'W') data.longitude = -data.longitude;

    // Speed in knots (convert to km/h)
    token = strtok(NULL, ",");
    if (token != NULL && strlen(token) > 0) {
        data.speed = atof(token) * KNOTS_TO_KMH;
    }
    
    return data;
}

void GPS_cleanup() {
    isRunning = false;  
    isInitialized = false;  
    pthread_cancel(gps_thread); 
    close(serial_port);
    printf("GPS cleanup\n");
}

bool GPS_hasSignal() {
    return signal;
}