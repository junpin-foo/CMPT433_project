/* joystick.c

    This file implements joystick functionality using an ADC chip over I2C.
    It initializes the I2C bus, reads joystick position data from the ADC,
    scales the raw values to a normalized range (-1 to 1), and provides a function
    to retrieve the current joystick position.
*/ 

#include "hal/joystick.h"
#include "hal/i2c.h"
#include <stdbool.h>
#include <assert.h>

#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x48 // ADC chip
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00
#define TLA2024_CHANNEL_CONF_0 0x83C2 // Configuration for Y-axis
#define TLA2024_CHANNEL_CONF_1 0x83D2 // Configuration for X-axis

static int i2c_file_desc = -1;
static bool isInitialized = false;

// Default Joystick values
static uint16_t x_min = 18, x_max = 1644;
static uint16_t y_min = 8, y_max = 1635;

void Joystick_initialize(void) {
    Ic2_initialize();
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    isInitialized = true;
}

void Joystick_cleanUp(void) {
    Ic2_cleanUp();
    isInitialized = false;
}

/*
This function scales the raw ADC values to a normalized range (-1 to 1).
*/
static double scale_value(double raw, double min, double max) {
    return 2.0 * ((raw - min) / (max - min)) - 1.0;
}

struct JoystickData Joystick_getReading() {
    if (!isInitialized) {
        fprintf(stderr, "Error: Joystick not initialized!\n");
        exit(EXIT_FAILURE);
    }

    write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, TLA2024_CHANNEL_CONF_0);
    uint16_t raw_y = read_i2c_reg16(i2c_file_desc, REG_DATA);
    uint16_t y_position = ((raw_y & 0xFF00) >> 8 | (raw_y & 0x00FF) << 8) >> 4;

    if (y_position < y_min) y_min = y_position;
    if (y_position > y_max) y_max = y_position;
    double y_scaled = scale_value(y_position, y_min, y_max) * -1;

    write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, TLA2024_CHANNEL_CONF_1);
    uint16_t raw_x = read_i2c_reg16(i2c_file_desc, REG_DATA);
    uint16_t x_position = ((raw_x & 0xFF00) >> 8 | (raw_x & 0x00FF) << 8) >> 4;

    if (x_position < x_min) x_min = x_position;
    if (x_position > x_max) x_max = x_position;
    double x_scaled = scale_value(x_position, x_min, x_max);

    // printf("Joystick current: X = %d, Y = %d\n", x_position, y_position);
    // printf("Joystick scaled current: X = %f, Y = %f\n", x_scaled, y_scaled);

    struct JoystickData data = {
        .x = x_scaled,
        .y = y_scaled,
        .isPressed = false // Placeholder
    };

    return data;
}