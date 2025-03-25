#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

// Global variables
pid_t recording_process = -1;
pthread_t record_thread;
int recording_active = 0;
char transcription_result[1024] = {0}; // To store transcription result

// Function to handle recording audio using arecord
void *record_audio(void *arg) {
    (void)arg; // Suppress unused parameter warning
    char *record_cmd[] = {
        "arecord", 
        "--device=hw:2,0", 
        "--format=S16_LE", 
        "--rate=44100", 
        "-c1", 
        "output.wav", 
        NULL
    };
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return NULL;
    }
    
    recording_process = fork();
    
    if (recording_process == -1) {
        perror("fork");
        return NULL;
    } else if (recording_process == 0) {
        // Child process
        close(pipefd[0]);  // Close unused read end
        dup2(pipefd[1], STDERR_FILENO);  // Redirect stderr to pipe
        dup2(pipefd[1], STDOUT_FILENO);  // Redirect stdout to pipe
        close(pipefd[1]);
        
        execvp(record_cmd[0], record_cmd);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        close(pipefd[1]);  // Close unused write end
        
        // Just wait until told to stop
        while (recording_active) {
            usleep(100000);  // Sleep for 100ms
        }
        
        // Terminate the recording process
        if (recording_process > 0) {
            kill(recording_process, SIGTERM);
            waitpid(recording_process, NULL, 0);
            recording_process = -1;
        }
    }
    
    return NULL;
}

// Function to start recording
int start_recording(void) {
    if (recording_active) {
        printf("Recording already in progress\n");
        return -1;
    }
    
    recording_active = 1;
    
    // Create thread to handle recording
    if (pthread_create(&record_thread, NULL, record_audio, NULL) != 0) {
        perror("pthread_create");
        recording_active = 0;
        return -1;
    }
    
    return 0;
}

// Function to stop recording
void stop_recording(void) {
    if (!recording_active) {
        return;
    }
    
    recording_active = 0;
    
    // Stop the recording process
    if (recording_process > 0) {
        kill(recording_process, SIGTERM);
        waitpid(recording_process, NULL, 0);
        recording_process = -1;
    }
    
    // Wait for recording thread to finish
    pthread_join(record_thread, NULL);
}


// Function to read command output
char* read_command_output(const char *cmd) {
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

// Function to transcribe audio using the Python script and return the result
char* transcribe_audio(void) {
    // Clear previous result
    memset(transcription_result, 0, sizeof(transcription_result));
    
    // Check if output.wav exists
    if (access("output.wav", F_OK) == -1) {
        printf("Error: No audio file found. Recording may have failed.\n");
        return NULL;
    }
    
    printf("Transcribing audio...\n");
    
    // Check if the Python script exists in the current directory
    if (access("speech.py", F_OK) == -1) {
        printf("Error: speech.py not found in the current directory.\n");
        return NULL;
    }
    
    // Run the Python script
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 ./speech.py output.wav");
    
    char *output = read_command_output(cmd);
    if (!output) {
        printf("Error: Failed to run speech recognition.\n");
        return NULL;
    }
    
    // Store the raw output as the transcription result
    if (output && strlen(output) > 0) {
        // Strip newlines and limit length
        char *newline = strchr(output, '\n');
        if (newline) *newline = '\0';
        
        if (strlen(output) < sizeof(transcription_result) - 1) {
            strcpy(transcription_result, output);
            free(output);
            return transcription_result;
        }
    }
    
    free(output);
    printf("Error: Failed to parse transcription result.\n");
    return NULL;
}

// Function that records, transcribes, and returns the speech content
char* get_speech_content(void) {
    // Start recording
    if (start_recording() != 0) {
        return NULL;
    }
    
    printf("Recording... Press Enter to stop.\n");
    getchar();  // Wait for user to press Enter
    
    // Stop recording
    stop_recording();
    
    // Transcribe and return the result
    return transcribe_audio();
}


// int main(int argc, char *argv[]) {
//     char input_buffer[32];
    
//     printf("Voice Recognition System\n");
//     printf("------------------------\n");
    
//     // Check if speech.py exists at startup
//     if (access("speech.py", F_OK) == -1) {
//         printf("Warning: speech.py not found in the current directory.\n");
//         printf("Make sure speech.py is in the same directory as this program.\n");
//     } else {
//         printf("Found speech.py in the current directory.\n");
//     }
    
//     printf("Commands:\n");
//     printf("  record - Start recording\n");
//     printf("  stop   - Stop recording\n");
//     printf("  quit   - Exit program\n\n");
    
//     while (1) {
//         printf("> ");
//         fflush(stdout);
        
//         if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
//             break;
//         }
        
//         // Remove trailing newline
//         input_buffer[strcspn(input_buffer, "\n")] = 0;
        
//         if (strcmp(input_buffer, "record") == 0) {
//             printf("Recording... (type 'stop' to stop)\n");
//             start_recording();
//         } else if (strcmp(input_buffer, "stop") == 0) {
//             if (recording_active) {
//                 stop_recording();
//                 char* result = transcribe_audio();
//                 if (result) {
//                     printf("Recognized Text: %s\n", result);
//                 }
//             } else {
//                 printf("Not currently recording.\n");
//             }
//         } else if (strcmp(input_buffer, "quit") == 0 || 
//                    strcmp(input_buffer, "exit") == 0) {
//             if (recording_active) {
//                 stop_recording();
//             }
//             break;
//         } else if (strlen(input_buffer) > 0) {
//             printf("Unknown command: %s\n", input_buffer);
//         }
//     }
    
//     printf("Exiting...\n");
//     return 0;
// }

