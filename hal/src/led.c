/* led.c
 * 
 * This file provides functions to initialize and clean up LED control, 
 * set LED triggers, brightness, and delays by interacting with the 
 * sysfs files for the LEDs.
 */

#include "hal/led.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

LED leds[] = {
    {"ACT"}, // Green LED
    {"PWR"}  // Red LED
};

static bool isInitialized = false;

void Led_initialize(void) {
    isInitialized = true;
}

void Led_cleanUp(void) {
    isInitialized = false;
}


void Led_setTrigger(LED *led, const char *trigger) {
    if (!isInitialized) {
        fprintf(stderr, "Error: Led not initialized!\n");
        exit(EXIT_FAILURE);
    }

    char trigger_file[256];
    snprintf(trigger_file, sizeof(trigger_file), "%s/%s/trigger", LED_FILE_NAME, led->name);

    FILE *pLedTriggerFile = fopen(trigger_file, "w");
    if (pLedTriggerFile == NULL) {
        perror("Error opening LED trigger file");
        exit(EXIT_FAILURE);
    }

    int charWritten = fprintf(pLedTriggerFile, trigger);
    if (charWritten <= 0) {
        perror("Error writing data to LED file");
        fclose(pLedTriggerFile);
        exit(EXIT_FAILURE);
    }

    fclose(pLedTriggerFile);
}

void Led_setBrightness(LED *led, int brightness) {
    if (!isInitialized) {
        fprintf(stderr, "Error: Led not initialized!\n");
        exit(EXIT_FAILURE);
    }

    char brightness_file[256];
    snprintf(brightness_file, sizeof(brightness_file), "%s/%s/brightness", LED_FILE_NAME, led->name);

    FILE *pLedBrightnessFile = fopen(brightness_file, "w");
    if (pLedBrightnessFile == NULL) {
        perror("Error opening LED brightness file");
        exit(EXIT_FAILURE);
    }

    int charWritten = fprintf(pLedBrightnessFile, "%d", brightness);
    if (charWritten <= 0) {
        perror("Error writing data to LED file");
        fclose(pLedBrightnessFile);
        exit(EXIT_FAILURE);
    }

    fclose(pLedBrightnessFile);
    // long seconds = 1;
    // long nanoseconds = 500000000;
    // struct timespec reqDelay = {seconds, nanoseconds};
    // nanosleep(&reqDelay, (struct timespec *) NULL);
}

void Led_setDelayOn(LED *led, int delay) {
    if (!isInitialized) {
        fprintf(stderr, "Error: Led not initialized!\n");
        exit(EXIT_FAILURE);
    }

    char delay_file[256];
    snprintf(delay_file, sizeof(delay_file), "%s/%s/delay_on", LED_FILE_NAME, led->name);

    FILE *pLedDelayFile = fopen(delay_file, "w");
    if (pLedDelayFile == NULL) {
        perror("Error opening LED delay_on file");
        exit(EXIT_FAILURE);
    }

    int charWritten = fprintf(pLedDelayFile, "%d", delay);
    if (charWritten <= 0) {
        perror("Error writing data to LED file");
        fclose(pLedDelayFile);
        exit(EXIT_FAILURE);
    }

    fclose(pLedDelayFile);
}

void Led_setDelayOff(LED *led, int delay) {
    if (!isInitialized) {
        fprintf(stderr, "Error: Led not initialized!\n");
        exit(EXIT_FAILURE);
    }
    
    char delay_file[256];
    snprintf(delay_file, sizeof(delay_file), "%s/%s/delay_off", LED_FILE_NAME, led->name);

    FILE *pLedDelayFile = fopen(delay_file, "w");
    if (pLedDelayFile == NULL) {
        perror("Error opening LED delay_off file");
        exit(EXIT_FAILURE);
    }

    int charWritten = fprintf(pLedDelayFile, "%d", delay);
    if (charWritten <= 0) {
        perror("Error writing data to LED file");
        exit(EXIT_FAILURE);
    }

    fclose(pLedDelayFile);
}
