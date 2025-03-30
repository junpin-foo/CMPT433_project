#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sleep_and_timer.h"
#include "hal/accelerometer.h"
#include "hal/i2c.h"
#include "hal/GPS.h"
#include "streetAPI.h"
#include "speedLimitLED.h"
#include "roadTracker.h"
#include "updateLcd.h"
#include "hal/gpio.h"
#include "hal/joystick.h"
#include "hal/microphone.h"
#include "hal/rotary_state.h"
#include "LED.h"


int main() {
    Ic2_initialize();
    // Initialize accelerometer and GPS
    Gpio_initialize();
    Joystick_initialize();
    Accelerometer_initialize();
    GPS_init();
    UpdateLcd_init();
    SpeedLED_init();
    StreetAPI_init();
    RoadTracker_init();
    // RoadTracker_setTarget("13450 102 Ave #250, Surrey, BC V3T 0A3");
    LED_init();
    while(1){
        sleepForMs(1000);
    }
    // Cleanup resources
    LED_cleanup();
    RoadTracker_cleanup();
    StreetAPI_cleanup();
    Ic2_cleanUp();
    Joystick_cleanUp();
    Gpio_cleanup();
    Accelerometer_cleanUp();
    return 0;
}



// // Flag to control program execution
// static int running = 1;

// int main() {
//     Gpio_initialize();
//     RotaryState_init();
//     Microphone_init();

    
//     // Start the button listener thread
//     if (Microphone_startButtonListener() == 0) {
//         printf("Button listener started. Press the rotary encoder button to record audio.\n");
//     } else {
//         printf("Failed to start button listener.\n");
//     }
    
//     // Main loop
//     while (running) {
        
//         sleep(1);
//     }
    
//     // Cleanup and exit
//     Microphone_stopButtonListener();
//     Microphone_cleanup();
//     RotaryState_cleanup();
//     Gpio_cleanup();
    
//     printf("Exiting...\n");
//     return 0;
// }

