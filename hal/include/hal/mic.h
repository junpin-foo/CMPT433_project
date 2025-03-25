#ifndef MIC_H
#define MIC_H

// Function to start recording audio
int start_recording(void);

// Function to stop recording audio
void stop_recording(void);

// Function to transcribe audio and return the transcription result
// Returns NULL if transcription fails
char* transcribe_audio(void);

// Function that handles the full recording and transcription process
// Returns the transcribed text or NULL if an error occurs
char* get_speech_content(void);

#endif // MIC_H