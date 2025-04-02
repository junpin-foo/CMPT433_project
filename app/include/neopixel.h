#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdbool.h>

// Initializes the neopixel controller and starts the neopxiel update thread
void NeoPixel_init(void);

// Cleans up the neopixel controller and joins the thread
void NeoPixel_cleanUp(void);
#endif // LED_CONTROLLER_H
