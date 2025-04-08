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
#include "neopixel.h"
#include "parking.h"
#include "hal/led.h"

// static int running = 1;
int main() {
    Ic2_initialize();
    Gpio_initialize();
    Joystick_initialize();
    Accelerometer_initialize();
    GPS_init();
    // Calling this will enable a thread read the gps data from demo_gps.txt. See "demo_locationData.txt" in project folder for more info"
    // GPS_demoInit();
    SpeedLED_init();
    StreetAPI_init();
    RoadTracker_init();
    Parking_init();
    NeoPixel_init();
    RotaryState_init();
    Microphone_init();

    // Start the button listener thread
    if (Microphone_startButtonListener() == 0) {
        printf("Button listener started. Press the rotary encoder button to record audio.\n");
    } else {
        printf("Failed to start button listener.\n");
    }
    
    while (true) {
        if (Joystick_isButtonPressed()) {
            break;
        } 
        sleepForMs(100);
    }

    Microphone_cleanup();
    NeoPixel_cleanUp();
    RoadTracker_cleanup();
    StreetAPI_cleanup();
    SpeedLED_cleanup();
    GPS_cleanup();
    Joystick_cleanUp();
    Parking_cleanup();
    Gpio_cleanup();
    Accelerometer_cleanUp();
    Ic2_cleanUp();
    return 0;
}



// Flag to control program execution
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

