/* lcd.c
* LCD module starts thread that updates the screen with the current frequency, dip count and max time every second.
*/

#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <updateLcd.h>


#define BUFFER_SIZE 100

static bool isInitialized = false;
static pthread_t lcdThread;
static volatile bool lcdRunning = true;

void Lcd_init()
{
    assert(!isInitialized);
    
    // Module Init
	UpdateLcd_init();
    // pthread_create(&lcdThread, NULL, &lcd_thread, NULL);
    isInitialized = true;
}
void Lcd_cleanup()
{
    assert(isInitialized);
    lcdRunning = false;
    pthread_join(lcdThread, NULL);
    UpdateLcd_cleanup();
    isInitialized = false;
}