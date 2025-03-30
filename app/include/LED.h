#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdbool.h>

// Initializes the LED controller and starts the LED update thread
void LED_init(void);

// Cleans up the LED controller and joins the thread
void LED_cleanup(void);
void LED_thread();
#endif // LED_CONTROLLER_H
