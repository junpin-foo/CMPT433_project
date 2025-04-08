#include "hal/i2c.h"
#include "hal/gpio.h"
#include "sleep_and_timer.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <math.h>
#include <pthread.h>
#include <roadTracker.h>
#include <speedLimitLED.h>
#include <parking.h>
#include "sharedDataLayout.h"
#include "neopixel.h"
#include "hal/led.h"
#include "hal/GPS.h"

// Memory mapping constants
#define ATCM_ADDR     0x79000000  // MCU ATCM (p59 TRM)
#define BTCM_ADDR     0x79020000  // MCU BTCM (p59 TRM)
#define MEM_LENGTH    0x8000

#define GREEN_LED &leds[0]
#define RED_LED &leds[1]
#define PROGRESS_PER_LED 14
#define MAX_NUM_LED 8
#define REMOVE_LED_MODE_NUMBER 10

static bool isInitialized = false;
static pthread_t LEDpthread;
static bool isRunning = false;

static void* LED_thread(void* arg);

// Memory mapping function
volatile void* getR5MmapAddr(void)
{
    // Access /dev/mem to gain access to physical memory (for memory-mapped devices/specialmemory)
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("ERROR: could not open /dev/mem; Did you run with sudo?");
        exit(EXIT_FAILURE);
    }

    // Inside main memory (fd), access the part at offset BTCM_ADDR:
    // (Get points to start of R5 memory after it's memory mapped)
    volatile void* pR5Base = mmap(0, MEM_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BTCM_ADDR);
    if (pR5Base == MAP_FAILED) {
        perror("ERROR: could not map memory");
        exit(EXIT_FAILURE);
    }
    close(fd);

    return pR5Base;
}

static void freeR5MmapAddr(volatile uint8_t* pR5Base)
{
    if (munmap((void*) pR5Base, MEM_LENGTH)) {
        perror("R5 munmap failed");
        exit(EXIT_FAILURE);
    }
}


void NeoPixel_init(void)
{
    assert(!isInitialized);
    isInitialized = true;
    isRunning = true;
    Led_initialize();
    if (pthread_create(&LEDpthread, NULL, LED_thread, NULL) != 0) {
        perror("Failed to create accelerometer thread");
        exit(EXIT_FAILURE);
    }
}

// Thread function
void* LED_thread(void* arg)
{
    (void)arg; // Mark the parameter as unused
    volatile void *pR5Base = getR5MmapAddr();
    // int progress = 0;
    while (isRunning) {
        // progress = (progress + 1)%9;
        if (GPS_hasSignal()) {
            Led_setTrigger(RED_LED, "none");
            Led_setBrightness(RED_LED, 0);
            Led_setTrigger(GREEN_LED, "none");
            Led_setBrightness(GREEN_LED, 1);

        } else {
            Led_setTrigger(GREEN_LED, "none");
            Led_setBrightness(GREEN_LED, 0);
            Led_setTrigger(RED_LED, "none");
            Led_setBrightness(RED_LED, 1);
        }
        if (!Parking_Activate()) {
            // printf("Parking not activated\n");
            MEM_UINT32((uint8_t*)pR5Base + MODE_OFFSET) = 0;
            // 0 to 8 in here
            int led_on = 1;
            double progress = RoadTracker_getProgress();
            if (progress >= 100) {
                led_on = MAX_NUM_LED;
            } else if (progress > 0) {
                led_on = (int)(progress / PROGRESS_PER_LED) + led_on;
            }
            int color = SpeedLED_getLEDColor();
            // int progress = 7;
            // printf("led_on: %d\n", led_on);
            // printf("    %15s: 0x%04x\n", "progress", MEM_UINT32((uint8_t*)pR5Base + PROGRESS_OFFSET));
            MEM_UINT32((uint8_t*)pR5Base + PROGRESS_OFFSET) = led_on;
            MEM_UINT32((uint8_t*)pR5Base + COLOR_OFFSET) = color;
        } else {
            // printf("Parking activated\n");
            // printf("modenumber: %d\n", Parking_getMode());
            // printf("color: %d\n", Parking_getMode());
            MEM_UINT32((uint8_t*)pR5Base + MODE_OFFSET) = Parking_getMode();
            MEM_UINT32((uint8_t*)pR5Base + COLOR_OFFSET) = Parking_getColor();
        }
        sleepForMs(1000);
    }
    // Remove LED effect by setting invalid mode number
    MEM_UINT32((uint8_t*)pR5Base + MODE_OFFSET) = REMOVE_LED_MODE_NUMBER;
    freeR5MmapAddr(pR5Base);
    return NULL;
}

void NeoPixel_cleanUp()
{
    assert(isInitialized);
    isRunning = false;
    Led_cleanUp();
    pthread_join(LEDpthread, NULL);
    isInitialized = false;
}
