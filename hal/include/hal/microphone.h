/**
 * hal/microphone.h
 * 
 * Microphone interface for audio recording and speech recognition
 */

 #ifndef HAL_MICROPHONE_H
 #define HAL_MICROPHONE_H
 
 #include <stdbool.h>
 
 // Initialize the microphone subsystem
 void Microphone_init(void);
 
 // Cleanup and free resources
 void Microphone_cleanup(void);
 
 // Start recording audio
 // Returns 0 on success, -1 on failure
 int Microphone_startRecording(void);
 
 // Stop recording audio
 // Returns 0 on success, -1 on failure
 int Microphone_stopRecording(void);
 
 // Check if currently recording
 bool Microphone_isRecording(void);
 
 // Transcribe the last recording and return the result
 // Returns NULL if transcription fails
 char* Microphone_transcribe(void);
 
 // Convenience function: record for specified duration and return transcription
 // Blocks until recording is complete
 // durationMs: recording duration in milliseconds (0 means manual stop)
 // Returns transcription result or NULL on failure
 char* Microphone_recordAndTranscribe(int durationMs);
 
 // Start the listener thread that monitors for rotary button presses
 // Returns 0 on success, -1 on failure
 int Microphone_startButtonListener(void);
 
 // Stop the listener thread
 void Microphone_stopButtonListener(void);
 
 // Register a callback function to be called when speech recognition completes
 // callback: function pointer to call with transcription result
 void Microphone_setTranscriptionCallback(void (*callback)(const char* transcription));
 
 // Update the sound threshold for detection
 // threshold: value to use for sound detection (higher = less sensitive)
 void Microphone_setSoundThreshold(int threshold);
 
 // Set whether transcription should happen automatically when recording stops
 void Microphone_setAutoTranscribe(bool enable);

 void Microphone_setLocationCallback(void (*callback)(const char* location));

 
 #endif // HAL_MICROPHONE_H