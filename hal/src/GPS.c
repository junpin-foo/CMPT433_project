#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "hal/GPS.h"
#include "stdbool.h"
#include "sleep_and_timer.h"

int serial_port;
static pthread_t gps_thread;
static pthread_mutex_t gps_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to protect current_location
static struct location current_location = {-1000, -1000, -1};  // Default invalid location
static bool isRunning = false;
static bool signal = false;
static char* GPS_read();
static struct location parse_GPRMC(char* gprmc_sentence);

// Function that runs in the thread to continuously read GPS data
static void* gps_thread_func(void* arg) {
    (void)arg;
    while (isRunning) {
        // Get the latest GPS data
        char* gps_data = GPS_read();
        // Parse the data into the location structure
        struct location new_location = parse_GPRMC(gps_data);

        // Update the global location safely using mutex
        pthread_mutex_lock(&gps_mutex);   // Lock the mutex before updating
        current_location = new_location;
        printf("current_location: Latitude %.6f, Longitude: %.6f, Speed: %.6f \n", current_location.latitude, current_location.longitude, current_location.speed);
        if (current_location.latitude == INVALID_LATITUDE) {
            signal = false;
            printf("NO GPS Signal !\n");
        } else {
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

    // Create the GPS thread that will continuously read and update the location
    pthread_create(&gps_thread, NULL, gps_thread_func, NULL);
}


static void* gps_thread2_func(void* arg) {
    (void)arg;
    while (isRunning) {
        pthread_mutex_lock(&gps_mutex);   // Lock the mutex before updating
        FILE* file = fopen("gps.txt", "r");
        if (file == NULL) {
            perror("Failed to open file");
            return 0; // failure
        }

        if (fscanf(file, "%lf %lf %lf", &current_location.latitude, &current_location.longitude, &current_location.speed) != 3) {
            fprintf(stderr, "Invalid file format. Expected 3 numbers.\n");
            fclose(file);
            return 0; // failure
        }
        printf("Thread %ld - Got: lat=%.2f, lon=%.2f, spd=%.2f\n", (long)arg, current_location.latitude, current_location.longitude, current_location.speed);
        pthread_mutex_unlock(&gps_mutex); // Unlock the mutex after updating
    }
    return NULL;
}

void GPS_demoInit() {
    signal = true;
    pthread_create(&gps_thread, NULL, gps_thread2_func, NULL);
}

void GPS_setLocation(struct location loc) {
    pthread_mutex_lock(&gps_mutex);   // Lock the mutex before updating
    current_location.latitude = loc.latitude;
    current_location.longitude = loc.longitude;
    current_location.speed = loc.speed;
    pthread_mutex_unlock(&gps_mutex); // Unlock the mutex after updating
}

struct location GPS_getLocation() {
    struct location loc;
    pthread_mutex_lock(&gps_mutex);   // Lock the mutex before reading
    loc = current_location;
    pthread_mutex_unlock(&gps_mutex); // Unlock the mutex after reading
    return loc;  // Return the most recent GPS location
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

static char* GPS_read() {
    static char read_buf[255];
    configure_serial(serial_port);
    while (isRunning) {  // Keep reading until we find a $GNGGA message
        int n = read(serial_port, &read_buf, sizeof(read_buf)); // Leave space for null terminator
        if (n > 0) {
            read_buf[n] = '\0'; // Properly terminate the string
            printf("here");
            // Check if the received message starts with "$GNRMC"
            if (strncmp(read_buf, "$GNRMC", 5) == 0) {
                return read_buf;
            }
        }
        sleepForMs(100);
    }
    return "";
}

static struct location parse_GPRMC(char* gnrmc_sentence) {
    char *token;
    char temp[255];
    struct location data = {-1000, -1000, -1};
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

    // Latitude
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return invalidData;
    double raw_lat = atof(token);
    int lat_deg = (int)(raw_lat / 100);
    double lat_min = raw_lat - (lat_deg * 100);
    data.latitude = lat_deg + (lat_min / 60.0);

    // N/S Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'S') data.latitude = -data.latitude;

    // Longitude
    token = strtok(NULL, ",");
    if (token == NULL || strlen(token) == 0) return invalidData;
    double raw_lon = atof(token);
    int lon_deg = (int)(raw_lon / 100);
    double lon_min = raw_lon - (lon_deg * 100);
    data.longitude = lon_deg + (lon_min / 60.0);

    // E/W Indicator
    token = strtok(NULL, ",");
    if (token && token[0] == 'W') data.longitude = -data.longitude;

    // Speed in knots (convert to km/h)
    token = strtok(NULL, ",");
    if (token != NULL && strlen(token) > 0) {
        data.speed = atof(token) * 1.852;
    }
    
    return data;
}

void GPS_cleanup() {
    isRunning = false;  // Stop the GPS thread
    pthread_cancel(gps_thread);  // Wait for the thread to finish
    close(serial_port);  // Close the serial port
    printf("GPS cleanup\n");
}

bool GPS_hasSignal() {
    return signal;
}