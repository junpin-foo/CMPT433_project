/**
 * hal/rotary_state.c
 * 
 * Implementation of the unified rotary encoder interface
 * Handles both rotation (A/B signals) and push button states
 */

 #include "hal/rotary_state.h"
 #include "hal/gpio.h"
 
 #include <gpiod.h>
 #include <assert.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <stdatomic.h>
 #include <unistd.h>
 #include <time.h>
 #include <errno.h>
 
 // Pin config for rotary encoder rotation (A/B)
 #define ROTARY_GPIO_CHIP      GPIO_CHIP_2
 #define ROTARY_A_PIN          7   // A = GPIO16 => line offset 7
 #define ROTARY_B_PIN          8   // B = GPIO17 => line offset 8
 
 // Pin config for rotary encoder push button
 #define PUSH_GPIO_CHIP        GPIO_CHIP_0
 #define PUSH_GPIO_LINE        10  // GPIO 24 => line offset 10
 
 // Bit positions for state encoding
 #define BIT_A 1  
 #define BIT_B 0  
 
 // States for rotary encoder (A/B)
 #define STATE_00 0  // A=0, B=0
 #define STATE_01 1  // A=0, B=1
 #define STATE_11 3  // A=1, B=1
 #define STATE_10 2  // A=1, B=0
 
 // Debounce time in nanoseconds
 #define DEBOUNCE_NS 5000000L

 // Counters
 static atomic_int rotaryCounter = 0;   // Counts rotation steps
 static atomic_int pushCounter = 0;     // Counts button presses
 static volatile int pulseCount = 0;    // For internal tracking
 
 // Initialization flags
 static bool rotaryInitialized = false;
 static bool pushInitialized = false;
 
 // GPIO handles
 static struct GpioLine* s_lineA = NULL;
 static struct GpioLine* s_lineB = NULL;
 static struct GpioLine* s_lineBtn = NULL;
 
 // State tracking
 static int lastRotaryState = -1;
 static struct gpiod_line_bulk s_bulkLines;
 static bool bulkInitialized = false;
 

 struct stateEvent {
     struct state* pNextState;
     void (*action)();
 };
 
 struct state {
     struct stateEvent rising;
     struct stateEvent falling;
 };
 
 // Button release action handler
 static void on_push_release(void)
 {
     pushCounter++;
 }
 
 // Push button states
 static struct state pushStates[] = {
     { // Not pressed
         .rising = {&pushStates[0], NULL},
         .falling = {&pushStates[1], NULL},
     },
 
     { // Pressed
         .rising = {&pushStates[0], on_push_release},
         .falling = {&pushStates[1], NULL},
     },
 };
 
 static struct state* pCurrentPushState = &pushStates[0];
 
 
 // Read the current state of A and B pins
 static int readRotaryState(void) {
     int a = 0;
     int b = 0;
     
     if (s_lineA != NULL) {
         a = Gpio_getLineValue(s_lineA);
     }
     
     if (s_lineB != NULL) {
         b = Gpio_getLineValue(s_lineB);
     }
     
     return (a << BIT_A) | (b << BIT_B);
 }
 
 // Detect rotation direction based on state transition with more robust approach
 static int detectDirection(int oldState, int newState) {
     // Reject invalid states
     if (oldState < 0 || oldState > 3 || newState < 0 || newState > 3) {
         return 0;
     }
     
     static const int directionTable[4][4] = {
         // New state:  00  01  10  11  <- 
        { 0, -1,  1,  0 },
        { 1,  0,  0, -1 },
        {-1,  0,  0,  1 },
        { 0,  1, -1,  0 }
     };
     
     return directionTable[oldState][newState];
 }
 
 // Rotary encoder action handlers
 static void on_clockwise(void) {
     rotaryCounter++;
     pulseCount++;
 }
 
 static void on_counterclockwise(void) {
     rotaryCounter--;
     pulseCount--;
 }
 

 // Initialize both rotary encoder components
 void RotaryState_init(void) {
     // Initialize rotary encoder (A/B)
     if (!rotaryInitialized) {
         // Make sure GPIO is initialized
         if (!Gpio_isInitialized()) {
             Gpio_initialize();
         }
         
         // Open both rotary encoder lines
         s_lineA = Gpio_openForEvents(ROTARY_GPIO_CHIP, ROTARY_A_PIN);
         if (!s_lineA) {
             fprintf(stderr, "Failed to open GPIO pin %d for rotary encoder A\n", ROTARY_A_PIN);
             exit(EXIT_FAILURE);
         }
         
         s_lineB = Gpio_openForEvents(ROTARY_GPIO_CHIP, ROTARY_B_PIN);
         if (!s_lineB) {
             fprintf(stderr, "Failed to open GPIO pin %d for rotary encoder B\n", ROTARY_B_PIN);
             Gpio_close(s_lineA);
             s_lineA = NULL;
             exit(EXIT_FAILURE);
         }
         
         gpiod_line_bulk_init(&s_bulkLines);
         gpiod_line_bulk_add(&s_bulkLines, (struct gpiod_line*)s_lineA);
         gpiod_line_bulk_add(&s_bulkLines, (struct gpiod_line*)s_lineB);
         bulkInitialized = true;
         
         // Request events on both lines
         if (gpiod_line_request_bulk_both_edges_events(&s_bulkLines, "rotary-encoder") < 0) {
             perror("Failed to request events on encoder lines");
             bulkInitialized = false;
             Gpio_close(s_lineA);
             s_lineA = NULL;
             Gpio_close(s_lineB);
             s_lineB = NULL;
             exit(EXIT_FAILURE);
         }
         
         // Read initial state
         lastRotaryState = readRotaryState();
         rotaryInitialized = true;
     }
     
     // Initialize push button
     if (!pushInitialized) {
         if (!Gpio_isInitialized()) {
             Gpio_initialize();
         }
         
         s_lineBtn = Gpio_openForEvents(PUSH_GPIO_CHIP, PUSH_GPIO_LINE);
         if (s_lineBtn) {
             pushInitialized = true;
         } else {
             fprintf(stderr, "Failed to open GPIO pin %d for push button\n", PUSH_GPIO_LINE);
         }
     }
 }
 
 // Clean up all resources
 void RotaryState_cleanup(void) {
     // Clean up rotary encoder
     if (rotaryInitialized) {
         if (bulkInitialized) {
             gpiod_line_release_bulk(&s_bulkLines);
             bulkInitialized = false;
         }
         
         if (s_lineA != NULL) {
             Gpio_close(s_lineA);
             s_lineA = NULL;
         }
         
         if (s_lineB != NULL) {
             Gpio_close(s_lineB);
             s_lineB = NULL;
         }
         
         rotaryInitialized = false;
     }
     
     // Clean up push button
     if (pushInitialized) {
         if (s_lineBtn != NULL) {
             Gpio_close(s_lineBtn);
             s_lineBtn = NULL;
         }
         pushInitialized = false;
     }
 }
 
 void RotaryState_doRotaryState(void) {
     if (!rotaryInitialized) {
         return;
     }
     
     static struct timespec lastEventTime = {0, 0};
     static int lastDir = 0; 
     static int sameDirectionCount = 0;  
     
     // Wait for events on either line
     struct gpiod_line_bulk eventBulk;
     gpiod_line_bulk_init(&eventBulk);
     
     // Set up a zero timeout for non-blocking operation
     struct timespec timeout = {0, 0};
     
     int ret = gpiod_line_event_wait_bulk(&s_bulkLines, &timeout, &eventBulk);
     
     if (ret <= 0) {
         // No events or error
         if (ret < 0 && ret != -ETIMEDOUT) {
             perror("Error waiting for events");
         }
         return;
     }
     
     // Get current time for debouncing
     struct timespec now;
     clock_gettime(CLOCK_MONOTONIC, &now);
     
     // Check all events
     for (unsigned int i = 0; i < gpiod_line_bulk_num_lines(&eventBulk); i++) {
         struct gpiod_line* changedLine = gpiod_line_bulk_get_line(&eventBulk, i);
         struct gpiod_line_event ev;
         
         if (gpiod_line_event_read(changedLine, &ev) < 0) {
             continue;
         }
         
         // Debounce: skip if event is too close to the previous one
         long diffNs = (now.tv_sec - lastEventTime.tv_sec) * 1000000000L + 
                      (now.tv_nsec - lastEventTime.tv_nsec);
         
         if (diffNs < DEBOUNCE_NS) {
             continue;
         }
         
         lastEventTime = now;
         
         // Read the current state
         int currentState = readRotaryState();
         
         // Skip if no actual change in state
         if (currentState == lastRotaryState || lastRotaryState == -1) {
             lastRotaryState = currentState;
             continue;
         }
         
         // Detect direction using helper function
         int direction = detectDirection(lastRotaryState, currentState);
         
         lastRotaryState = currentState;
         
         // If the direction matches the previous direction, increase confidence
         if (direction != 0) {
             if (direction == lastDir) {
                 sameDirectionCount++;
             } else {
                 // Direction changed - reset counter and set new direction
                 lastDir = direction;
                 sameDirectionCount = 1;
             }
             
             // Only trigger when we have enough confidence that we are turning
             if (sameDirectionCount >= 1) {
                 if (direction == 1) {
                     on_clockwise();
                 } else if (direction == -1) {
                     on_counterclockwise();
                 }
             }
         }
     }
 }
 
 // Process push button state
 void RotaryState_doPushState(void) {
     if (!pushInitialized) {
         return;
     }
     
     struct gpiod_line_bulk bulkEvents;
     int numEvents = Gpio_waitForLineChange(s_lineBtn, &bulkEvents);
     
     // Iterate over the events
     for (int i = 0; i < numEvents; i++) {
         // Get the line handle for this event
         struct gpiod_line* line_handle = gpiod_line_bulk_get_line(&bulkEvents, i);
         
         // Get the line event
         struct gpiod_line_event event;
         if (gpiod_line_event_read(line_handle, &event) == -1) {
             perror("Line Event");
             continue;
         }
         
         // Run the state machine
         bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;
         
         struct stateEvent* pStateEvent = NULL;
         if (isRising) {
             pStateEvent = &pCurrentPushState->rising;
         } else {
             pStateEvent = &pCurrentPushState->falling;
         } 
         
         // Do the action
         if (pStateEvent->action != NULL) {
             pStateEvent->action();
         }
         pCurrentPushState = pStateEvent->pNextState;
     }
 }
 
 // Get the current rotation counter value
 int RotaryState_getRotaryValue(void) {
     return rotaryCounter;
 }
 
 // Get the current push counter value
 int RotaryState_getPushValue(void) {
     return pushCounter;
 }
 
 // Reset all counters
 void RotaryState_reset(void) {
     atomic_store(&rotaryCounter, 0);
     atomic_store(&pushCounter, 0);
     pulseCount = 0;
 }