/**
 * ai_api.c
 *
 * Implementation of the C wrapper for the Gemini AI API
 */

 #include "ai_api.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <pthread.h>
 
 // Constants
 #define MAX_RESPONSE_SIZE 4096
 #define MAX_CMD_SIZE 2048
 
 // Global variables
 static pthread_mutex_t ai_mutex = PTHREAD_MUTEX_INITIALIZER;
 static char ai_response_buffer[MAX_RESPONSE_SIZE] = {0};

 // Read the output that comes from running the commands
 static char* read_command_output(const char *cmd) {
     FILE *fp;
     char *output = NULL;
     size_t output_size = 0;
     char buffer[1024];
     
     fp = popen(cmd, "r");
     if (fp == NULL) {
         perror("Failed to run command");
         return NULL;
     }
     
     while (fgets(buffer, sizeof(buffer), fp) != NULL) {
         size_t buffer_len = strlen(buffer);
         char *new_output = realloc(output, output_size + buffer_len + 1);
         if (new_output == NULL) {
             free(output);
             pclose(fp);
             return NULL;
         }
         output = new_output;
         strcpy(output + output_size, buffer);
         output_size += buffer_len;
     }
     
     pclose(fp);
     return output;
 }
 
 // Initialize the AI API
 void AI_init(void) {
     pthread_mutex_lock(&ai_mutex);
     memset(ai_response_buffer, 0, sizeof(ai_response_buffer));
     pthread_mutex_unlock(&ai_mutex);
     
     // Check if API key is set
     if (!AI_isApiKeySet()) {
         printf("Warning: GEMINI_API_KEY environment variable is not set.\n");
         printf("Please set it with: export GEMINI_API_KEY='insert_api_key'\n");
     }
 }
 
 // Clean up the AI API
 void AI_cleanup(void) {
     pthread_mutex_lock(&ai_mutex);
     pthread_mutex_unlock(&ai_mutex);
 }
 
 // Process the transcription with the AI API
 char* AI_processTranscription(void) {
     pthread_mutex_lock(&ai_mutex);
     memset(ai_response_buffer, 0, sizeof(ai_response_buffer));
     pthread_mutex_unlock(&ai_mutex);
     
     // Prepare command to run the Python script
     char cmd[MAX_CMD_SIZE];
     snprintf(cmd, sizeof(cmd), "python3 ./ai_api.py");
     
     // Run the command and get output
     char *output = read_command_output(cmd);
     if (!output) {
         printf("Error: Failed to get AI response.\n");
         return NULL;
     }
     
     // Store the response in buffer
     pthread_mutex_lock(&ai_mutex);
     
     if (strlen(output) < sizeof(ai_response_buffer) - 1) {
         strcpy(ai_response_buffer, output);
         pthread_mutex_unlock(&ai_mutex);
         free(output);
         return ai_response_buffer;
     }
     
     pthread_mutex_unlock(&ai_mutex);
     free(output);
     printf("Error: AI response too large to store.\n");
     return NULL;
 }
 
 // Process a specific text with the AI API
 char* AI_processText(const char* text) {
     if (!text || strlen(text) == 0) {
         printf("Error: Empty text provided for AI processing.\n");
         return NULL;
     }
     
     // Create a temporary file with the provided text
     char temp_file[128];
     snprintf(temp_file, sizeof(temp_file), "app_output/temp_input.txt");
     
     // Ensure directory exists
     char mkdir_cmd[256];
     snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p app_output");
     system(mkdir_cmd);
     
     // Write text to temporary file
     FILE *fp = fopen(temp_file, "w");
     if (!fp) {
         perror("Failed to create temporary input file");
         return NULL;
     }
     
     fprintf(fp, "%s", text);
     fclose(fp);
     
     // Run the Python script with the input file
     pthread_mutex_lock(&ai_mutex);
     memset(ai_response_buffer, 0, sizeof(ai_response_buffer));
     pthread_mutex_unlock(&ai_mutex);
     
     char cmd[MAX_CMD_SIZE];
     snprintf(cmd, sizeof(cmd), "python3 ./ai_api.py %s", temp_file);
     
     char *output = read_command_output(cmd);
     if (!output) {
         printf("Error: Failed to get AI response.\n");
         unlink(temp_file);  // Clean up temp file
         return NULL;
     }
     
     // Store the response
     pthread_mutex_lock(&ai_mutex);
     
     if (strlen(output) < sizeof(ai_response_buffer) - 1) {
         strcpy(ai_response_buffer, output);
         
         pthread_mutex_unlock(&ai_mutex);
         free(output);
         unlink(temp_file);  // Clean up temp file
         return ai_response_buffer;
     }
     
     pthread_mutex_unlock(&ai_mutex);
     free(output);
     unlink(temp_file);  // Clean up temp file
     printf("Error: AI response too large to store.\n");
     return NULL;
 }

 
 // Check if the API key is set
 bool AI_isApiKeySet(void) {
     // Check if the environment variable exists
     char *api_key = getenv("GEMINI_API_KEY");
     return (api_key != NULL && strlen(api_key) > 0);
 }