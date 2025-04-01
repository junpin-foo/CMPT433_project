// /* joystick.c
//     This file implements joystick functionality using an ADC chip over I2C.
//     It initializes the I2C bus, reads joystick position data from the ADC,
//     scales the raw values to a normalized range (-1 to 1), and provides a function
//     to retrieve the current joystick position.
// */ 

#include "hal/joystick.h"
#include "hal/i2c.h"
#include "hal/gpio.h"
#include "sleep_and_timer.h"
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdatomic.h>

#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x48 // ADC chip
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00
#define TLA2024_CHANNEL_CONF_0 0x83C2 // Configuration for Y-axis
#define TLA2024_CHANNEL_CONF_1 0x83D2 // Configuration for X-axis
#define X_Y_UP_THRESHOLD 0.7
#define X_Y_DOWN_THRESHOLD -0.7

#define GPIO_CHIP GPIO_CHIP_2
#define GPIO_LINE 15
struct GpioLine* s_line = NULL;

static int i2c_file_desc = -1;
static bool isInitialized = false;
static atomic_int page_number = 1;
static atomic_int isButtonPressed = 0;

static pthread_t button_thread;
static volatile bool keepReading = false;

static uint16_t x_min = 18, x_max = 1644;
static uint16_t y_min = 8, y_max = 1635;

//DEBOUNCE
#define DEBOUNCE_TIME_MS 100
static struct timespec last_btn_time;
void *joystick_button_thread_func(void *arg);

void Joystick_initialize(void) {
    s_line = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE);
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    keepReading = true;
    isInitialized = true;
    pthread_create(&button_thread, NULL, joystick_button_thread_func, NULL);
}

void Joystick_cleanUp(void) {
    keepReading = false;
    pthread_join(button_thread, NULL);
    Gpio_close(s_line);
    isInitialized = false;
}

void *joystick_button_thread_func(void *arg) {
    (void)arg;
    assert(isInitialized);

    clock_gettime(CLOCK_MONOTONIC, &last_btn_time);

    while (keepReading) {
        struct gpiod_line_bulk events;
        bool buttonFlag = false;
        
        int eventCount = Gpio_waitFor1LineChange(s_line, &events);
        if (eventCount > 0) {
            struct gpiod_line_event event;
            struct gpiod_line *line_handle = gpiod_line_bulk_get_line(&events, 0);
            if (gpiod_line_event_read(line_handle, &event) == -1) {
                perror("Line Event");
                exit(EXIT_FAILURE);
            }

            if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
                buttonFlag = true;
            }
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        if (buttonFlag && time_diff_ms(&last_btn_time, &now) > DEBOUNCE_TIME_MS) {
            int new_page = atomic_load(&page_number) % 2 + 1;
            atomic_store(&isButtonPressed, 1);
            atomic_store(&page_number, new_page);
            printf("Button pressed");
            last_btn_time = now;
        } else {
            atomic_store(&isButtonPressed, 0);
            last_btn_time = now;
        }

        sleepForMs(10);
    }
    return NULL;
}

static double scale_value(double raw, double min, double max) {
    return 2.0 * ((raw - min) / (max - min)) - 1.0;
}

JoystickDirection getJoystickDirection(void) {
    struct JoystickData data = Joystick_getReading();
    if (data.x > X_Y_UP_THRESHOLD) return JOYSTICK_RIGHT;
    if (data.x < X_Y_DOWN_THRESHOLD) return JOYSTICK_LEFT;
    if (data.y > X_Y_UP_THRESHOLD) return JOYSTICK_UP;
    if (data.y < X_Y_DOWN_THRESHOLD) return JOYSTICK_DOWN;
    return JOYSTICK_CENTER;
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

    struct JoystickData data = {
        .x = x_scaled,
        .y = y_scaled,
        .isPressed = false //Handled in button thread
    };

    return data;
}

int Joystick_getPageCount() {
    return page_number;
}

int Joystick_isButtonPressed() {
    return isButtonPressed;
}