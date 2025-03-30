
/**
 * hal/rotary_state.h
 * 
 * Unified interface for rotary encoder with push button
 * Handles both rotation (A/B signals) and push button states
 */

 #ifndef ROTARY_STATE_H
 #define ROTARY_STATE_H
 
 // Initialization and cleanup
 void RotaryState_init(void);
 void RotaryState_cleanup(void);
 
 // State machine processing 
 void RotaryState_doRotaryState(void);  // Process rotary encoder state
 void RotaryState_doPushState(void);    // Process push button state
 
 // Value getters
 int RotaryState_getRotaryValue(void);  // Get current rotation counter
 int RotaryState_getPushValue(void);    // Get current push counter
 void RotaryState_reset(void);          // Reset all counters
 
 #endif // ROTARY_STATE_H