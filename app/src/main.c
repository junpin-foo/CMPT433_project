#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // For sleep()
#include "hal/accelerometer.h"
#include "hal/GPS.h"
#include <math.h>

#define SAMPLING_PERIOD_MS 100   // Sampling period in milliseconds
#define SAMPLING_PERIOD_SEC 0.1  // Sampling period in seconds
#define GRAVITY 9.81             // Earth's gravity in m/s²
#define ACCEL_THRESHOLD 0.12      // Ignore noise below this threshold
#define DECAY_FACTOR 0.95        // Slow down car speed naturally

float car_speed = 0.0;  // Initial speed

// Function to compute acceleration after removing gravity
float getHorizontalAcceleration(AccelerometerData data) {
    // Convert raw accelerometer values to G (±8g full scale)
    float ax_g = (data.x / 32768.0) * 8.0;
    float ay_g = (data.y / 32768.0) * 8.0;
    // float az_g = (data.z / 32768.0) * 8.0;

    // Convert from G to m/s²
    float ax = ax_g * GRAVITY;
    float ay = ay_g * GRAVITY;
    // float az = az_g * GRAVITY;

    // Remove Earth's gravity from Z-axis
    // float az_corrected = az - GRAVITY;

    // Compute horizontal acceleration
    float accel_horizontal = sqrt(ax * ax + ay * ay);

    // Ignore small vibrations (noise filter)
    if (accel_horizontal < ACCEL_THRESHOLD) {
        return 0;
    }

    return accel_horizontal;
}

// int main() {
//     // Initialize accelerometer
//     Accelerometer_initialize();
//     printf("Accelerometer initialized. Reading values...\n");

//     while (1) {
//         // Get accelerometer reading
//         AccelerometerData data = Accelerometer_getReading();

        
//         // Compute acceleration
//         float accel_horizontal = getHorizontalAcceleration(data);

//         // Compute speed change (v = a * t)
//         float speed_change = accel_horizontal * SAMPLING_PERIOD_SEC;

//         // Update speed
//         if (accel_horizontal > 0) {
//             car_speed += speed_change;  // Accelerate
//         } else {
//             car_speed *= DECAY_FACTOR;  // Apply friction
//         }

//         // Convert to km/h
//         float speed_kmh = car_speed * 3.6;
        

//         // Print values
//         printf("X: %d, Y: %d, Z: %d\n", data.x, data.y, data.z);
        
//         printf("Acceleration: %.2f m/s²\n", accel_horizontal);
//         printf("Speed Change: %.2f m/s\n", speed_change);
//         printf("Current Speed: %.2f m/s (%.2f km/h)\n", car_speed, speed_kmh);
//         printf("Current Speed (INT): %.d km/h\n", (int) speed_kmh);
        

//         // Sleep for the sampling period
//         usleep(SAMPLING_PERIOD_MS * 1000);
//     }

//     // Cleanup resources
//     Accelerometer_cleanUp();
//     return 0;
// }

int main() {
    GPS_init();
    while (1) {
        char* message = GPS_read();
        printf("Message received: %s\n", message);
        double latitude = 0.0, longitude = 0.0;
        parse_GNGGA(message, &latitude, &longitude);
        printf("Latitude: %lf, Longitude: %lf\n", latitude, longitude);
        sleep(1);
    }
    return 0;
}