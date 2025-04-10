/* 
 * This header defines the interface for controlling NeoPixel LEDs and onboard GPIO LEDs
 * based on GPS signal status, road tracking progress, and parking mode.
 * 
 * It Uses a background thread to update LED states every second and communicates to R5 to control NeoPixel animations.
 * Integrates with the RoadTracker and Parking modules for dynamic behavior.
**/
#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdbool.h>

// Initializes the neopixel controller and starts the neopxiel update thread
void NeoPixel_init(void);

// Cleans up the neopixel controller and joins the thread
void NeoPixel_cleanUp(void);
#endif // LED_CONTROLLER_H
