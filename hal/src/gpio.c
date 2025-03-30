#include "hal/gpio.h"
#include <stdlib.h>
#include <stdio.h>
#include <gpiod.h>
#include <assert.h>

// Relies on the gpiod library.
// Installation for cross compiling:
//      (host)$ sudo dpkg --add-architecture arm64
//      (host)$ sudo apt update
//      (host)$ sudo apt install libgpdiod-dev:arm64
// GPIO: https://www.ics.com/blog/gpio-programming-exploring-libgpiod-library
// Example: https://github.com/starnight/libgpiod-example/blob/master/libgpiod-input/main.c


static bool s_isInitialized = false;

static char* s_chipNames[] = {
    "gpiochip0",
    "gpiochip1",
    "gpiochip2",
};

// Hold open chips
static struct gpiod_chip* s_openGpiodChips[GPIO_NUM_CHIPS];

void Gpio_initialize(void)
{
    for (int i = 0; i < GPIO_NUM_CHIPS; i++) {
         // Open GPIO chip
        s_openGpiodChips[i] = gpiod_chip_open_by_name(s_chipNames[i]);
        if (!s_openGpiodChips[i]) {
            perror("GPIO Initializing: Unable to open GPIO chip");
            exit(EXIT_FAILURE);
        }
    }
    s_isInitialized = true;
}

void Gpio_cleanup(void)
{
    assert(s_isInitialized);
    for (int i = 0; i < GPIO_NUM_CHIPS; i++) {
        // Close GPIO chip
        gpiod_chip_close(s_openGpiodChips[i]);
        if (!s_openGpiodChips[i]) {
            perror("GPIO Cleanup: Unable to close GPIO chip");
            exit(EXIT_FAILURE);
        }
    }
    s_isInitialized = false;
}


struct GpioLine* Gpio_openForEvents(enum eGpioChips chip, int pinNumber)
{
    assert(s_isInitialized);
    struct gpiod_chip* gpiodChip = s_openGpiodChips[chip];
    struct gpiod_line* line = gpiod_chip_get_line(gpiodChip, pinNumber);
    if (!line) {
        perror("Unable to get GPIO line");
        exit(EXIT_FAILURE);
    }

    return (struct GpioLine*) line;
}

void Gpio_close(struct GpioLine* line)
{
    assert(s_isInitialized);
    gpiod_line_release((struct gpiod_line*) line);
}

// Needed to work with sameer's rotary encoder code
int Gpio_waitFor1LineChange(
    struct GpioLine* line1, 
    struct gpiod_line_bulk *bulkEvents
) {
    assert(s_isInitialized);

    // Source: https://people.eng.unimelb.edu.au/pbeuchat/asclinic/software/building_block_gpio_encoder_counting.html   
    struct gpiod_line_bulk bulkWait;
    gpiod_line_bulk_init(&bulkWait);
    
    gpiod_line_bulk_add(&bulkWait, (struct gpiod_line*)line1);
    
    gpiod_line_request_bulk_both_edges_events(&bulkWait, "Event Waiting");

    struct timespec timeout = { 1, 0 }; // 1 second timeout
    int result = gpiod_line_event_wait_bulk(&bulkWait, &timeout, bulkEvents);
    if ( result == -1) {
        perror("Error waiting on lines for event waiting");
        return -1;
    }
    if (result == 0) {
        return 0;
    }

    int numEvents = gpiod_line_bulk_num_lines(bulkEvents);
    return numEvents;
}

// Needed to work with sameer's rotary encoder code
int Gpio_waitFor2LineChange(
    struct GpioLine* line1, 
    struct GpioLine* line2, 
    struct gpiod_line_bulk *bulkEvents
) {
    assert(s_isInitialized);

    // Source: https://people.eng.unimelb.edu.au/pbeuchat/asclinic/software/building_block_gpio_encoder_counting.html   
    struct gpiod_line_bulk bulkWait;
    gpiod_line_bulk_init(&bulkWait);
    
    gpiod_line_bulk_add(&bulkWait, (struct gpiod_line*)line1);
    gpiod_line_bulk_add(&bulkWait, (struct gpiod_line*)line2);
    
    gpiod_line_request_bulk_both_edges_events(&bulkWait, "Event Waiting");

    struct timespec timeout = { 1, 0 }; // 1 second timeout
    int result = gpiod_line_event_wait_bulk(&bulkWait, &timeout, bulkEvents);
    if ( result == -1) {
        perror("Error waiting on lines for event waiting");
        return -1;
    }
    if (result == 0) {
        return 0;
    }

    int numEvents = gpiod_line_bulk_num_lines(bulkEvents);
    return numEvents;
}

// Needed to work with sameer's rotary encoder code
int Gpio_waitForLineChange(
    struct GpioLine* line1, 
    struct gpiod_line_bulk *bulkEvents
) {
    assert(s_isInitialized);

    // Source: https://people.eng.unimelb.edu.au/pbeuchat/asclinic/software/building_block_gpio_encoder_counting.html   
    struct gpiod_line_bulk bulkWait;
    gpiod_line_bulk_init(&bulkWait);
    
    // TODO: Add more lines if needed
    gpiod_line_bulk_add(&bulkWait, (struct gpiod_line*)line1);
    
    gpiod_line_request_bulk_both_edges_events(&bulkWait, "Event Waiting");

    int result = gpiod_line_event_wait_bulk(&bulkWait, NULL, bulkEvents);
    if (result == -1) {
        perror("Error waiting on lines for event waiting");
        exit(EXIT_FAILURE);
    }

    int numEvents = gpiod_line_bulk_num_lines(bulkEvents);
    return numEvents;
}

// Needed to work with sameer's rotary encoder code
bool Gpio_isInitialized(void)
{
    return s_isInitialized;
}

// Needed to work with sameer's rotary encoder code
int Gpio_getLineValue(struct GpioLine* line)
{
    assert(s_isInitialized);
    int value = gpiod_line_get_value((struct gpiod_line*) line);
    if (value < 0) {
        perror("Failed to get line value");
        exit(EXIT_FAILURE);
    }
    return value;
}