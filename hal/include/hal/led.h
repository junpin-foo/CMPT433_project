/* led.h
 * 
 * This file declares a structure for defining LEDs and functions for initializing, 
 * cleaning up, and controlling LED attributes like trigger, brightness, 
 * and delays using sysfs files. The `LED_FILE_NAME` macro defines the 
 * path to the sysfs LED directory.
 */

#ifndef _LED_H_
#define _LED_H_

#include <stdio.h>
#include <stdlib.h>

#define LED_FILE_NAME "/sys/class/leds"

typedef struct {
    const char *name; //(e.g., "ACT", "PWR")
} LED;

extern LED leds[];


void Led_initialize(void);
void Led_cleanUp(void);
/*
This function sets the trigger for the LED to the specified trigger value.
    @Param trigger: type of trigger. E.g. timer, heartbeat, none.
*/
void Led_setTrigger(LED *led, const char *trigger);
/*
This function sets the brightness of the LED to the specified value.
    @Param brightness: 1 for on, 0 for off.
*/
void Led_setBrightness(LED *led, int brightness);
/*
This function sets the delay_on value for the LED to the specified value.
    @Param delay: delay in milliseconds to keep LED on.
*/
void Led_setDelayOn(LED *led, int on);
/*
This function sets the delay_off value for the LED to the specified value.
    @Param delay: delay in milliseconds to keep LED off
*/
void Led_setDelayOff(LED *led, int off);

#endif