#include "hal/i2c.h"
#include "hal/gpio.h"
#include "hal/lcd.h"
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

#include "sharedDataLayout.h"

// Memory mapping constants
#define ATCM_ADDR     0x79000000  // MCU ATCM (p59 TRM)
#define BTCM_ADDR     0x79020000  // MCU BTCM (p59 TRM)
#define MEM_LENGTH    0x8000

static bool isInitialized = false;
static pthread_t LEDpthread;

void* LED_thread(void* arg);

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


void LED_init(void)
{
    assert(!isInitialized);
    isInitialized = true;

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
    while (true) {
        // progress = (progress + 1)%9;
        int progress = RoadTracker_getProgress();
        // int progress = 7;
        // printf("    %15s: 0x%04x\n", "progress", MEM_UINT32((uint8_t*)pR5Base + PROGRESS_OFFSET));
        MEM_UINT32((uint8_t*)pR5Base + PROGRESS_OFFSET) = progress;
        sleepForMs(1000);
    }
    // return NULL;
}

void LED_cleanup(void)
{
    assert(isInitialized);
    pthread_join(LEDpthread, NULL);
    isInitialized = false;
}
