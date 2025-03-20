/* accelerometer.c
 * 
 * This file provides a HAL for interfacing with an
 * accelerometer over I2C using the existing I2C functions.
 */

#include "hal/accelerometer.h"
#include "hal/i2c.h"
#include "sleep_and_timer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h> 
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#define I2C_BUS "/dev/i2c-1"
#define ACCEL_I2C_ADDRESS 0x19

// Register addresses (example for an LIS3DH accelerometer)
#define REG_WHO_AM_I 0x0F   // WHO_AM_I register address

#define REG_CTRL1  0x20
#define REG_CTRL2  0x21
#define REG_CTRL3  0x22
#define REG_CTRL4  0x23
#define REG_CTRL5  0x24
#define REG_CTRL6  0x25
#define REG_FIFO_CTRL 0x2E

#define REG_OUT_X_L 0x28
#define REG_OUT_X_H 0x29
#define REG_OUT_Y_L 0x2A
#define REG_OUT_Y_H 0x2B
#define REG_OUT_Z_L 0x2C
#define REG_OUT_Z_H 0x2D

#define SENSITIVITY_2G 4096.0  // Sensitivity for ±2g range (14-bit resolution)

static int i2c_file_desc = -1;
static bool isInitialized = false;
static volatile bool keepReading = false;
// static pthread_t accelerometer_thread;

//PROTOTYPES
void Accelerometer_initialize(void);
void Accelerometer_cleanUp(void);
void *Accelerometer_thread_func(void *arg);
AccelerometerData Accelerometer_getReading(void);

void Accelerometer_initialize(void) {
    i2c_file_desc = init_i2c_bus(I2C_BUS, ACCEL_I2C_ADDRESS);
    isInitialized = true;
    keepReading = true;
    write_i2c_reg8(i2c_file_desc, REG_CTRL1, 0x97);  //100Hz, (High)14-bit resolution, (Low)14-bit resolution 
    write_i2c_reg8(i2c_file_desc, REG_CTRL6, 0x00); //+2g
}

void Accelerometer_cleanUp(void) {
    keepReading = false;
    isInitialized = false;
}


AccelerometerData Accelerometer_getReading(void) {
    if (!isInitialized) {
        fprintf(stderr, "Error: Accelerometer not initialized!\n");
        exit(EXIT_FAILURE);
    }

    uint8_t raw_data[6];
    read_i2c_burst(i2c_file_desc, REG_OUT_X_L, raw_data, 6);

    int16_t x = (int16_t)((raw_data[1] << 8) | raw_data[0]) >> 2;
    int16_t y = (int16_t)((raw_data[3] << 8) | raw_data[2]) >> 2;
    int16_t z = (int16_t)((raw_data[5] << 8) | raw_data[4]) >> 2;

    sleepForMs(10);
    AccelerometerData data = {x / SENSITIVITY_2G, y / SENSITIVITY_2G, z / SENSITIVITY_2G};
    return data;
}
