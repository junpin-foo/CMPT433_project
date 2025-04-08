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

// // Global flag to control program execution.
// // Note: Defined without static so that it is visible to other files.
// volatile int running = 1;
// static volatile int force_exit_counter = 0;

// void on_location_received(const char* formatted_location) {
//     printf("Setting navigation target to: %s\n", formatted_location);
//     char location_copy[256];
//     strncpy(location_copy, formatted_location, sizeof(location_copy) - 1);
//     location_copy[sizeof(location_copy) - 1] = '\0';
//     RoadTracker_setTarget(location_copy);
// }

// void on_clear_target_received(void) {
//     printf("Clearing navigation target\n");
//     RoadTracker_resetTarget();
// }

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

// // Alarm handler for forced exit
// void handle_alarm(int sig) {
//     (void)sig;  // Unused parameter
//     printf("\nShutdown timeout reached. Forcing exit...\n");
//     _exit(1);
// }

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


// Flag to control program execution
static int running = 1;

int main() {
    Gpio_initialize();
    RotaryState_init();
    Microphone_init();

    
    // Start the button listener thread
    if (Microphone_startButtonListener() == 0) {
        printf("Button listener started. Press the rotary encoder button to record audio.\n");
    } else {
        printf("Failed to start button listener.\n");
    }

    // Main loop
    while (running) {

        sleep(1);
    }

    // Cleanup and exit
    Microphone_stopButtonListener();
    Microphone_cleanup();
    RotaryState_cleanup();
    Gpio_cleanup();

    printf("Exiting...\n");
    return 0;
}