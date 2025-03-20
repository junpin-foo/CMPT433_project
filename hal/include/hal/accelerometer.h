/* accelerometer.h 
 * 
 * This file declares functions for initializing and cleaning up the I2C interface, 
 * opening an I2C bus, and performing 16-bit register read/write operations.
 * 
 * These methods are taken from class guide: I2C Guide
 */

#ifndef _ACCELEROMETER_H_
#define _ACCELEROMETER_H_

#include <stdint.h>

typedef struct {
    double x;
    double y;
    double z;
} AccelerometerData;

void Accelerometer_initialize(void);
void Accelerometer_cleanUp(void);
AccelerometerData Accelerometer_getReading();
uint8_t Accelerometer_readWhoAmI(void);

#endif