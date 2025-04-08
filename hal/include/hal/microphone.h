/**
 * hal/microphone.h
 * 
 * Interface for microphone functionality
 */

 #ifndef MICROPHONE_H
 #define MICROPHONE_H
 
 #include <stdbool.h>
 
 // Global flag to indicate program shutdown
 extern volatile int shutting_down;
 
 // Initialize the microphone
 void Microphone_init(void);
 
 // Cleanup and free resources
 void Microphone_cleanup(void);
 
 // Start recording audio
 int Microphone_startRecording(void);
 
 // Stop recording audio
 int Microphone_stopRecording(void);
 
 // Check if currently recording
 bool Microphone_isRecording(void);
 
 // Set whether to auto-transcribe when recording stops
 void Microphone_setAutoTranscribe(bool enable);
 
 // Transcribe the last recording and return the result
 char* Microphone_transcribe(void);
 
 // Record for specified duration and return transcription
 char* Microphone_recordAndTranscribe(int durationMs);
 
 // Start the listener thread that monitors for rotary button presses
 int Microphone_startButtonListener(void);
 
 // Stop the listener thread
 void Microphone_stopButtonListener(void);
 
 // Register transcription callback
 void Microphone_setTranscriptionCallback(void (*callback)(const char* transcription));
 
 // Register location callback
 void Microphone_setLocationCallback(void (*callback)(const char* formatted_location));
 
 // Register clear target callback
 void Microphone_setClearTargetCallback(void (*callback)(void));
 
 // Register shutdown command callback
 void Microphone_setShutdownCallback(void (*callback)(void));
 
 // Update the sound threshold for detection
 void Microphone_setSoundThreshold(int threshold);
 
 #endif // MICROPHONE_H