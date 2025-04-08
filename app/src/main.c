#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>  /* For waitpid */
#include <string.h>
#include <pthread.h>
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
    // GPS_init();
    // Calling this will enable a thread read the gps data from demo_gps.txt. See "demo_locationData.txt" in project folder for more info"
    GPS_demoInit();
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

// // Add the voice control shutdown callback
// void on_shutdown_command_received(void) {
//     printf("Voice command shutdown received!\n");
//     running = 0;  // Set the running flag to false
    
    
//     printf("Initiating graceful shutdown...\n");
// }

// void handle_signal(int sig) {
//     printf("\nReceived signal %d, initiating shutdown...\n", sig);
    
//     force_exit_counter++;
//     if (force_exit_counter >= 3) {
//         printf("Multiple interrupts received, forcing immediate exit!\n");
//         killpg(getpgrp(), SIGKILL);
//         _exit(1);
//     }
    
//     running = 0;
//     printf("Sending SIGTERM to process group %d...\n", getpgrp());
//     killpg(getpgrp(), SIGTERM);
//     printf("Setting 3-second backup exit timer...\n");
//     alarm(3);
// }

// Flag to control program execution
// static int running = 1;

// int main() {
//     if (setpgid(0, 0) < 0) {
//         perror("Failed to set process group");
//         exit(EXIT_FAILURE);
//     }
    
//     struct sigaction sa;
//     memset(&sa, 0, sizeof(sa));
//     sa.sa_handler = handle_signal;
//     sigaction(SIGINT, &sa, NULL);
//     sigaction(SIGTERM, &sa, NULL);
//     sigaction(SIGQUIT, &sa, NULL);

//     struct sigaction sa_alarm;
//     memset(&sa_alarm, 0, sizeof(sa_alarm));
//     sa_alarm.sa_handler = handle_alarm;
//     sigaction(SIGALRM, &sa_alarm, NULL);
    
//     sigset_t old_mask;
//     sigemptyset(&old_mask);
//     pthread_sigmask(SIG_SETMASK, &old_mask, NULL);
    
//     Ic2_initialize();
//     Gpio_initialize();
//     Joystick_initialize();
//     Accelerometer_initialize();
//     GPS_demoInit();
//     StreetAPI_init();
//     SpeedLED_init();
//     RoadTracker_init();
//     Parking_init();
//     NeoPixel_init();
//     RotaryState_init();
//     Microphone_init();
    
//     Microphone_setLocationCallback(on_location_received);
//     Microphone_setClearTargetCallback(on_clear_target_received);
    
//     Microphone_setShutdownCallback(on_shutdown_command_received);
    
    
//     if (Microphone_startButtonListener() == 0) {
//         printf("Voice command system active. Press the rotary encoder button to record commands.\n");
//         printf("Say 'Hey Beagle set target to [location]' to change navigation target.\n");
//         printf("Say 'Hey Beagle shutdown' to exit the program.\n");
//     } else {
//         printf("Failed to start voice command system.\n");
//     }

//     while (running) {
//         if (Joystick_isButtonPressed()) {
//             // RoadTracker_setTarget("13450 102 Ave #250, Surrey, BC V3T 0A3");
//             on_shutdown_command_received(); 

//         }
//         sleepForMs(100);
//     }
    
//     printf("Starting cleanup...\n");
    
//     killpg(getpgrp(), SIGTERM);
    
//     Microphone_cleanup();
//     RotaryState_cleanup();
//     // NeoPixel_cleanup();
//     // Parking_cleanup();
//     // UpdateLcd_cleanup();
//     RoadTracker_cleanup();
//     SpeedLED_cleanup();
//     StreetAPI_cleanup();
//     // GPS_cleanup();
//     Accelerometer_cleanUp();
//     Joystick_cleanUp();
//     Gpio_cleanup();
//     Ic2_cleanUp();
    
//     printf("Application terminated gracefully.\n");
//     return 0;
// }
