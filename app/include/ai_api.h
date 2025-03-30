/**
 * ai_api.h
 * 
 * C wrapper for the Gemini AI API interface
 */

 #ifndef AI_API_H
 #define AI_API_H
 
 #include <stdbool.h>
 

// Initialize the AI API module
 void AI_init(void);
 
 /**
  * Clean up the AI API module
  */
 void AI_cleanup(void);
 
// Process transcription from the mic
 char* AI_processTranscription(void);
 
// Process the text
 char* AI_processText(const char* text);
 
 // Set the callback function to be called when AI processing completes
//  void AI_setResponseCallback(void (*callback)(const char* response));
 
 // Checks if API key is set
 bool AI_isApiKeySet(void);

static char* read_command_output(const char *cmd);
 
 #endif /* AI_API_H */