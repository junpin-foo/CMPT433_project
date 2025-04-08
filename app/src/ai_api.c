/**
 * ai_api.c
 *
 * Implementation of the C wrapper for the Gemini AI API,
 * using fork/exec with a nonblocking pipe and select() so that
 * the function checks regularly for the global “running” flag.
 */

 #include "ai_api.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <fcntl.h>      /* For fcntl(), O_NONBLOCK */
 #include <errno.h>
 #include <signal.h>
 #include <time.h>
 #include <sys/wait.h>
 #include <sys/select.h>
 
 #define MAX_RESPONSE_SIZE 4096
 
 // Global mutex and response buffer.
 static pthread_mutex_t ai_mutex = PTHREAD_MUTEX_INITIALIZER;
 static char ai_response_buffer[MAX_RESPONSE_SIZE] = {0};
 
 // The global flag defined in main.c (ensure it is declared with external linkage).
//  extern volatile int running;

 static char* run_python_script(const char* arg) {
     int pipefd[2];
     if (pipe(pipefd) < 0) {
         perror("pipe");
         return NULL;
     }
     pid_t pid = fork();
     if (pid < 0) {
         perror("fork");
         close(pipefd[0]);
         close(pipefd[1]);
         return NULL;
     }
     
     if (pid == 0) {
         // Child process.
         // Put the child into its own process group.
         if (setpgid(0, 0) < 0) {
             perror("setpgid");
             exit(1);
         }
         close(pipefd[0]); // Close reading end.
         // Redirect stdout to the writing end of the pipe.
         if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
             perror("dup2");
             exit(1);
         }
         close(pipefd[1]);
         // Execute the Python script with an argument if provided.
         if (arg)
             execlp("python3", "python3", "./ai_api.py", arg, (char*)NULL);
         else
             execlp("python3", "python3", "./ai_api.py", (char*)NULL);
         perror("execlp failed");
         exit(1);
     }
     
     // Parent process.
     close(pipefd[1]); // Close the writing end.
     // Set the pipe to nonblocking mode.
     int flags = fcntl(pipefd[0], F_GETFL, 0);
     fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
 
     char *output = NULL;
     size_t output_size = 0;
     char buffer[1024];
     time_t start_time = time(NULL);
     const int TIMEOUT_SECONDS = 10;
 
     while ((time(NULL) - start_time) < TIMEOUT_SECONDS) {
        //  if (!running) {
        //      // Interrupt detected: kill the child process group.
        //      killpg(pid, SIGKILL);
        //      break;
        //  }
         fd_set readfds;
         FD_ZERO(&readfds);
         FD_SET(pipefd[0], &readfds);
         struct timeval tv;
         tv.tv_sec = 0;
         tv.tv_usec = 50000;  // 50ms timeout.
         int ret = select(pipefd[0] + 1, &readfds, NULL, NULL, &tv);
         if (ret > 0 && FD_ISSET(pipefd[0], &readfds)) {
             ssize_t count = read(pipefd[0], buffer, sizeof(buffer) - 1);
             if (count > 0) {
                 buffer[count] = '\0';
                 char *new_output = realloc(output, output_size + count + 1);
                 if (new_output == NULL) {
                     free(output);
                     close(pipefd[0]);
                     waitpid(pid, NULL, 0);
                     return NULL;
                 }
                 output = new_output;
                 memcpy(output + output_size, buffer, count + 1);
                 output_size += count;
             }
         }
         // Check if the child process has finished.
         int status;
         pid_t result = waitpid(pid, &status, WNOHANG);
         if (result > 0)
             break;
     }
     // If a timeout was exceeded, kill the child process group.
     if ((time(NULL) - start_time) >= TIMEOUT_SECONDS) {
         killpg(pid, SIGKILL);
     }
     // Wait for the child process to avoid zombie processes.
     waitpid(pid, NULL, 0);
     close(pipefd[0]);
 
     if (output == NULL)
         return strdup("");
     return output;
 }
 
 // Initialize the AI API.
 void AI_init(void) {
     pthread_mutex_lock(&ai_mutex);
     memset(ai_response_buffer, 0, sizeof(ai_response_buffer));
     pthread_mutex_unlock(&ai_mutex);
     if (!AI_isApiKeySet()) {
         printf("Warning: GEMINI_API_KEY environment variable is not set.\n");
         printf("Please set it with: export GEMINI_API_KEY='insert_api_key'\n");
     }
 }
 
 // Clean up the AI API.
 void AI_cleanup(void) {
     pthread_mutex_lock(&ai_mutex);
     pthread_mutex_unlock(&ai_mutex);
 }
 
 // Process the transcription with the AI API.
 char* AI_processTranscription(void) {
     pthread_mutex_lock(&ai_mutex);
     memset(ai_response_buffer, 0, sizeof(ai_response_buffer));
     pthread_mutex_unlock(&ai_mutex);
 
     char *output = run_python_script(NULL);
     if (!output) {
         printf("Error: Failed to get AI response.\n");
         return NULL;
     }
 
     // Store the response in the internal buffer.
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
 
 // Process a specific text with the AI API.
 char* AI_processText(const char* text) {
     if (!text || strlen(text) == 0) {
         printf("Error: Empty text provided for AI processing.\n");
         return NULL;
     }
 
     // Write the text to a temporary file.
     char temp_file[128];
     snprintf(temp_file, sizeof(temp_file), "app_output/temp_input.txt");
     char mkdir_cmd[256];
     snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p app_output");
     system(mkdir_cmd);
 
     FILE *fp = fopen(temp_file, "w");
     if (!fp) {
         perror("Failed to create temporary input file");
         return NULL;
     }
     fprintf(fp, "%s", text);
     fclose(fp);
 
     pthread_mutex_lock(&ai_mutex);
     memset(ai_response_buffer, 0, sizeof(ai_response_buffer));
     pthread_mutex_unlock(&ai_mutex);
 
     char *output = run_python_script(temp_file);
     if (!output) {
         printf("Error: Failed to get AI response.\n");
         unlink(temp_file);  // Clean up temp file.
         return NULL;
     }
 
     pthread_mutex_lock(&ai_mutex);
     if (strlen(output) < sizeof(ai_response_buffer) - 1) {
         strcpy(ai_response_buffer, output);
         pthread_mutex_unlock(&ai_mutex);
         free(output);
         unlink(temp_file);
         return ai_response_buffer;
     }
     pthread_mutex_unlock(&ai_mutex);
     free(output);
     unlink(temp_file);
     printf("Error: AI response too large to store.\n");
     return NULL;
 }
 
 // Check if the API key is set.
 bool AI_isApiKeySet(void) {
     char *api_key = getenv("GEMINI_API_KEY");
     return (api_key != NULL && strlen(api_key) > 0);
 }
 