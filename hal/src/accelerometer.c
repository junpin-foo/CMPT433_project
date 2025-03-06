/* accelerometer.c
 * 
 * This file provides a HAL for interfacing with an
 * accelerometer over I2C using the existing I2C functions.
 */

#include "hal/accelerometer.h"
#include "hal/i2c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h> 

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

static int i2c_file_desc = -1;
static bool isInitialized = false;

void Accelerometer_initialize(void) {
    Ic2_initialize();
    i2c_file_desc = init_i2c_bus(I2C_BUS, ACCEL_I2C_ADDRESS);
    isInitialized = true;

    // Example: Configure accelerometer 
    write_i2c_reg8(i2c_file_desc, REG_CTRL1, 0x57);  //100Hz, (High)14-bit resolution, (Low)14-bit resolution 
    // write_i2c_reg8(i2c_file_desc, REG_CTRL3, 0x03);  //no sleep
    // write_i2c_reg8(i2c_file_desc, REG_CTRL4, 0x01);  //Data-Ready is routed to INT1 pad
    // write_i2c_reg8(i2c_file_desc, REG_CTRL5, 0x01);
    write_i2c_reg8(i2c_file_desc, REG_CTRL6, 0x24); //+8g
    // write_i2c_reg8(i2c_file_desc, REG_CTRL6, 0x00); //+2g
    // write_i2c_reg8(i2c_file_desc, REG_CTRL2, 0x40); //soft reset

    uint8_t fifo_config = (0x00 << 3) | (0x00);  // FMode = 110, FTH = 0x0A
    write_i2c_reg8(i2c_file_desc, REG_FIFO_CTRL, fifo_config);
}

void Accelerometer_cleanUp(void) {
    Ic2_cleanUp();
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

    // Print in binary
    // printf("X: 0b");
    // for (int i = 13; i >= 0; i--) printf("%d", (x >> i) & 1);
    // printf(" (%" PRIx16 "h)\n", x);

    // printf("Y: 0b");
    // for (int i = 13; i >= 0; i--) printf("%d", (y >> i) & 1);
    // printf(" (%" PRIx16 "h)\n", y);

    // printf("Z: 0b");
    // for (int i = 13; i >= 0; i--) printf("%d", (z >> i) & 1);
    // printf(" (%" PRIx16 "h)\n", z);


    AccelerometerData data = {x, y, z};

    return data;
}

uint8_t Accelerometer_readWhoAmI(void) {
    if (!isInitialized) {
        fprintf(stderr, "Error: Accelerometer not initialized!\n");
        return 0x00;
    }

    uint8_t who_am_i = read_i2c_reg8(i2c_file_desc, REG_WHO_AM_I);
    printf("WHO_AM_I Register: 0x%X\n", who_am_i);

    if (who_am_i == 0x44) {
        printf("IIS2DLPC detected successfully!\n");
    } else {
        printf("Error: Unexpected WHO_AM_I value (expected 0x44, got 0x%X)\n", who_am_i);
    }

    return who_am_i;
}
