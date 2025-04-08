/**
 * hal/microphone.c
 * 
 * Implementation of the microphone interface for audio recording and speech recognition
 */

 #include "hal/microphone.h"
 #include "hal/rotary_state.h"
 #include "hal/gpio.h"
 #include "ai_api.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <signal.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <sys/wait.h>
 #include <time.h>
 #include <fcntl.h>
 #include <errno.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <ctype.h>
 
 // Global variables
 static pthread_t record_thread;
 static pthread_t button_listener_thread;
 static int recording_active = 0;
 static int listener_active = 0;
 static time_t last_sound_time = 0;
 static int consecutive_silent_frames = 0;  // Count of consecutive silent frames
 static int auto_transcribe_on_stop = 1;
 static char transcription_result[1024] = {0}; // To store transcription result
 static char output_folder[256] = "app_output"; // Folder to save output files
 static char output_filename[256] = "output.wav"; // Audio output filename
 static char transcription_filename[256] = "transcribed_output.txt"; // Transcription output filename
 static void (*transcription_callback)(const char* transcription) = NULL;
 static void (*location_callback)(const char* formatted_location) = NULL;
 static pid_t arecord_pid = -1;
 static volatile int shutting_down = 0;
 
 static const int MAX_RECORDING_DURATION = 15;  // 15 seconds max recording duration
 static const int SILENCE_TIMEOUT = 2;  // Silence timeout in seconds
 static const int SOUND_THRESHOLD = 50;  // Sound detection threshold
 static const int SILENT_FRAMES_REQUIRED = 100;  // Required frames of silence
 
 // Mutex for thread synchronization
 static pthread_mutex_t mic_mutex = PTHREAD_MUTEX_INITIALIZER;
 static pthread_cond_t mic_cond = PTHREAD_COND_INITIALIZER;

 // Add this function to microphone.c to extract just the formatted address
// Add it before the record_audio function

static char* extract_formatted_address(const char* ai_response) {
    if (!ai_response || !*ai_response) {
        printf("Empty AI response\n");
        return NULL;
    }
    
    printf("Raw AI response: [%s]\n", ai_response);
    
    // First look for a newline character
    const char* newline = strchr(ai_response, '\n');
    if (newline && *(newline + 1) != '\0') {
        // There's content after the newline
        newline++; // Skip the newline
        
        // Check if there's actual content (not just whitespace)
        const char* line_start = newline;
        while (*line_start && isspace((unsigned char)*line_start)) {
            line_start++;
        }
        
        if (*line_start) {
            // Found content after newline, make a copy
            char* result = strdup(line_start);
            if (result) {
                // Trim trailing whitespace
                int len = strlen(result);
                while (len > 0 && isspace((unsigned char)result[len-1])) {
                    result[--len] = '\0';
                }
                
                printf("Extracted address using newline: [%s]\n", result);
                return result;
            }
        }
    }
    
    // Alternative approach: look for the address at the end of the response
    // This is more specific to the format we've seen
    const char* end_of_prompt = strstr(ai_response, "Do not include any other text in your response.");
    if (end_of_prompt) {
        end_of_prompt += strlen("Do not include any other text in your response.");
        
        // Skip whitespace
        while (*end_of_prompt && isspace((unsigned char)*end_of_prompt)) {
            end_of_prompt++;
        }
        
        if (*end_of_prompt) {
            char* result = strdup(end_of_prompt);
            if (result) {
                // Trim trailing whitespace
                int len = strlen(result);
                while (len > 0 && isspace((unsigned char)result[len-1])) {
                    result[--len] = '\0';
                }
                
                printf("Extracted address using prompt end: [%s]\n", result);
                return result;
            }
        }
    }
    
    // If all else fails, check for anything that looks like an address format
    const char* address_pattern = strchr(ai_response, '8');  // Assuming 8888 is there
    if (address_pattern) {
        // Check if this is the start of a number
        if (isdigit((unsigned char)address_pattern[0]) && 
            isdigit((unsigned char)address_pattern[1]) && 
            isdigit((unsigned char)address_pattern[2]) && 
            isdigit((unsigned char)address_pattern[3])) {
            
            // Likely found the address
            char* result = strdup(address_pattern);
            if (result) {
                // Trim trailing whitespace
                int len = strlen(result);
                while (len > 0 && isspace((unsigned char)result[len-1])) {
                    result[--len] = '\0';
                }
                
                printf("Extracted address using pattern match: [%s]\n", result);
                return result;
            }
        }
    }
    
    printf("Could not extract address\n");
    return NULL;
}
 
 // Case-insensitive character comparison
 static int char_icmp(char a, char b) {
     return tolower((unsigned char)a) - tolower((unsigned char)b);
 }
 
 // Case-insensitive string search
 static const char* my_strcasestr(const char* haystack, const char* needle) {
     if (!haystack || !needle) return NULL;
     
     size_t needle_len = strlen(needle);
     if (needle_len == 0) return haystack;
     
     for (; *haystack; haystack++) {
         if (char_icmp(*haystack, *needle) == 0) {
             const char* h = haystack;
             const char* n = needle;
             size_t i = 0;
             
             while (i < needle_len && *h && char_icmp(*h, *n) == 0) {
                 h++;
                 n++;
                 i++;
             }
             
             if (i == needle_len) {
                 return haystack;
             }
         }
     }
     
     return NULL;
 }
 
 // Function to parse location from transcription
 static char* parse_location(const char* transcription) {
     // Check if transcription contains the trigger phrase
     const char* trigger = "hey beagle set target to";
     const char* pos = my_strcasestr(transcription, trigger);
     
     if (pos) {
         // Move past the trigger phrase to get the target location
         pos += strlen(trigger);
         
         // Skip any leading whitespace
         while (*pos == ' ' || *pos == '\t') {
             pos++;
         }
         
         // If there's location text after the trigger
         if (*pos) {
             return strdup(pos);
         }
     }
     
     return NULL; // No location found
 }
 
 // Function to read command output with timeout
 static char* read_command_output_with_timeout(const char *cmd, int timeout_seconds) {
     char *output = NULL;
     size_t output_size = 0;
     char buffer[1024];
     int pipes[2];
     pid_t pid;
     
     // Create a pipe for reading command output
     if (pipe(pipes) != 0) {
         perror("pipe");
         return NULL;
     }
     
     pid = fork();
     if (pid < 0) {
         perror("fork");
         close(pipes[0]);
         close(pipes[1]);
         return NULL;
     }
     
     if (pid == 0) {
         // Child process
         close(pipes[0]);
         dup2(pipes[1], STDOUT_FILENO);
         close(pipes[1]);
         
         execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
         perror("execl");
         exit(EXIT_FAILURE);
     }
     
     // Parent process
     close(pipes[1]);
     
     // Set non-blocking mode for reading
     int flags = fcntl(pipes[0], F_GETFL, 0);
     fcntl(pipes[0], F_SETFL, flags | O_NONBLOCK);
     
     time_t start_time = time(NULL);
     
     while (time(NULL) - start_time < timeout_seconds) {
         // Check if we need to stop due to shutdown
         if (shutting_down) {
             kill(pid, SIGTERM);
             waitpid(pid, NULL, 0);
             close(pipes[0]);
             free(output);
             return NULL;
         }
         
         ssize_t n = read(pipes[0], buffer, sizeof(buffer) - 1);
         if (n > 0) {
             buffer[n] = '\0';
             char *new_output = realloc(output, output_size + n + 1);
             if (new_output == NULL) {
                 free(output);
                 close(pipes[0]);
                 kill(pid, SIGTERM);
                 waitpid(pid, NULL, 0);
                 return NULL;
             }
             output = new_output;
             memcpy(output + output_size, buffer, n);
             output_size += n;
             output[output_size] = '\0';
         } else if (n == 0) {
             // End of file
             break;
         } else if (errno != EAGAIN) {
             // Error
             perror("read");
             free(output);
             close(pipes[0]);
             kill(pid, SIGTERM);
             waitpid(pid, NULL, 0);
             return NULL;
         }
         
         // Short sleep to avoid high CPU usage
         usleep(10000);
     }
     
     close(pipes[0]);
     
     // If we reached the timeout, kill the process
     if (time(NULL) - start_time >= timeout_seconds) {
         printf("Command timed out after %d seconds\n", timeout_seconds);
         kill(pid, SIGTERM);
     }
     
     waitpid(pid, NULL, 0);
     
     if (output == NULL) {
         return strdup("");
     }
     
     return output;
 }
 
 // Signal handler for arecord
 static void handle_arecord_signal(int sig) {
     (void)sig; // Suppress unused parameter warning
     
     if (arecord_pid > 0) {
         kill(arecord_pid, SIGTERM);
         arecord_pid = -1;
     }
 }
 
 // Function to handle recording audio and monitoring sound levels
 static void *record_audio(void *arg) {
     int duration_ms = *((int*)arg);
     free(arg);
     
     // Register signal handler for cleanup
     signal(SIGINT, handle_arecord_signal);
     
     // Ensure output directory exists
     char mkdir_cmd[512];
     snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", output_folder);
     system(mkdir_cmd);
     
     // Prepare full output path
     char output_path[512];
     snprintf(output_path, sizeof(output_path), "%s/%s", output_folder, output_filename);
     
     // Set up the arecord command with the output path
     // Using a named pipe to split the audio stream
     char path[512];
     snprintf(path, sizeof(path), "%s/audio_fifo", output_folder);
     
     // Create a named pipe (FIFO)
     unlink(path); // Remove any existing pipe
     if (mkfifo(path, 0666) == -1) {
         perror("mkfifo");
         pthread_exit(NULL);
     }
     
     // Command to record audio and write to both the WAV file and the path
     char cmd[2048];
    //  snprintf(cmd, sizeof(cmd), 
    //          "arecord --device=hw:3,0 --format=S16_LE --rate=44100 -c1 | tee %s > %s",
    //          path, output_path);

    snprintf(cmd, sizeof(cmd), 
        "arecord --device=hw:2,0 --format=S16_LE --rate=44100 -c2 | tee %s > %s",
        path, output_path);
     
     // Start the recording process
     int pipe_fd[2];
     if (pipe(pipe_fd) == -1) {
         perror("pipe");
         unlink(path);
         pthread_exit(NULL);
     }
     
     arecord_pid = fork();
     if (arecord_pid == -1) {
         perror("fork");
         close(pipe_fd[0]);
         close(pipe_fd[1]);
         unlink(path);
         pthread_exit(NULL);
     }
     
     if (arecord_pid == 0) {
         // Child process (arecord)
         close(pipe_fd[0]);
         dup2(pipe_fd[1], STDERR_FILENO);
         close(pipe_fd[1]);
         
         execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
         perror("execl");
         exit(EXIT_FAILURE);
     }
     
     // Parent process
     close(pipe_fd[1]);
     
     // Open the FIFO for reading audio data for sound detection
     int fifo_fd = open(path, O_RDONLY | O_NONBLOCK);
     if (fifo_fd == -1) {
         perror("open fifo");
         close(pipe_fd[0]);
         kill(arecord_pid, SIGTERM);
         waitpid(arecord_pid, NULL, 0);
         unlink(path);
         arecord_pid = -1;
         pthread_exit(NULL);
     }
     
     // Initialize time of last sound and silence counter
     last_sound_time = time(NULL);
     consecutive_silent_frames = 0;
     
     // Buffer to read audio samples
     short buffer[1024];
     ssize_t read_size;
     
     // If duration is specified, set end time
     struct timespec end_time;
     if (duration_ms > 0) {
         clock_gettime(CLOCK_MONOTONIC, &end_time);
         end_time.tv_sec += duration_ms / 1000;
         end_time.tv_nsec += (duration_ms % 1000) * 1000000;
         if (end_time.tv_nsec >= 1000000000) {
             end_time.tv_sec += 1;
             end_time.tv_nsec -= 1000000000;
         }
     }
     
     // Get start time for maximum recording duration
     struct timespec start_time;
     clock_gettime(CLOCK_MONOTONIC, &start_time);
     
     // Process audio data until told to stop
     while (recording_active && !shutting_down) {
         // Check if any duration is met
         if (duration_ms > 0) {
             struct timespec current_time;
             clock_gettime(CLOCK_MONOTONIC, &current_time);
             if (current_time.tv_sec > end_time.tv_sec || 
                 (current_time.tv_sec == end_time.tv_sec && current_time.tv_nsec >= end_time.tv_nsec)) {
                 printf("Specified duration reached, stopping recording...\n");
                 pthread_mutex_lock(&mic_mutex);
                 recording_active = 0;
                 pthread_mutex_unlock(&mic_mutex);
                 break;
             }
         }
         
         // Check if we've reached the maximum recording duration
         struct timespec current_time;
         clock_gettime(CLOCK_MONOTONIC, &current_time);
         long elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
         
         if (elapsed_seconds >= MAX_RECORDING_DURATION) {
             printf("Maximum recording duration (%d seconds) reached, stopping...\n", MAX_RECORDING_DURATION);
             pthread_mutex_lock(&mic_mutex);
             recording_active = 0;
             pthread_mutex_unlock(&mic_mutex);
             break;
         }
         
         // Read from FIFO
         read_size = read(fifo_fd, buffer, sizeof(buffer));
         
         if (read_size > 0) {
             // Calculate energy level (simple sum of absolute values)
             long energy = 0;
             size_t sample_count = read_size / sizeof(short);
             for (size_t i = 0; i < sample_count; i++) {
                 energy += abs(buffer[i]);
             }
             
             if (sample_count > 0) {
                 energy /= sample_count;  // Average energy
             }
             
             // If energy reading is above threshold, it counts as proper sound
             if (energy > SOUND_THRESHOLD) {
                 last_sound_time = time(NULL);
                 consecutive_silent_frames = 0;  // Reset silent frame counter
                 
                 // Only print for loud sounds to reduce console spam
                 if (energy > SOUND_THRESHOLD * 2) {
                     // printf("Sound detected! Level: %ld\n", energy);
                 }
             } else {
                 // Count consecutive silent frames
                 consecutive_silent_frames++;
                 
                 // Only check for timeout if we have enough consecutive silent frames
                 if (consecutive_silent_frames >= SILENT_FRAMES_REQUIRED) {
                     time_t current_time = time(NULL);
                     double silent_time = difftime(current_time, last_sound_time);
                     
                     if (silent_time >= SILENCE_TIMEOUT) {
                         printf("Silence detected for %.1f seconds (%d frames), stopping recording...\n", 
                                silent_time, consecutive_silent_frames);
                         pthread_mutex_lock(&mic_mutex);
                         recording_active = 0;
                         pthread_mutex_unlock(&mic_mutex);
                         break;
                     }
                 }
             }
         } else if (read_size == -1 && errno != EAGAIN) {
             // Error reading from FIFO
             perror("read fifo");
             break;
         }
         
         // Short sleep to avoid high CPU usage
         usleep(10000);  // 10ms sleep
     }
     
     // Clean up recording
     close(fifo_fd);
     close(pipe_fd[0]);
     
     if (arecord_pid > 0) {
         kill(arecord_pid, SIGTERM);
         waitpid(arecord_pid, NULL, 0);
         arecord_pid = -1;
     }
     
     unlink(path);
     
     // Only transcribe if we're not shutting down
     if (!shutting_down && auto_transcribe_on_stop) {
         printf("Recording stopped, starting transcription...\n");
         
         // Small delay to ensure file is properly closed
         usleep(500000);  // 0.5 seconds
         
         // Call transcribe function directly
         char* result = Microphone_transcribe();
         if (result && !shutting_down) {
             printf("Auto-transcription result: %s\n", result);
             
             // Check if this is a location setting request
             char* location = parse_location(result);
             if (location) {
                 printf("Location detected: %s\n", location);
                 
                 // Create a formatted query for the AI to get address details
                 char location_query[1024];
                 snprintf(location_query, sizeof(location_query), 
                         "Provide only the full address in standard format for: %s. Format should be like: 8888 University Dr W, Burnaby, BC V5A 1S6. Do not include any other text in your response.", location);
                         
                 // Process with AI API for location formatting
                 printf("Getting formatted address for location...\n");
                 char* ai_response = AI_processText(location_query);
                 if (ai_response && !shutting_down) {
                    printf("AI response: %s\n", ai_response);
                    
                    // Extract just the formatted address part
                    char* clean_address = extract_formatted_address(ai_response);
                    
                    if (clean_address && *clean_address) {  // Make sure it's not empty
                        printf("Setting navigation target to: [%s]\n", clean_address);
                        
                        // Call the location callback if registered
                        if (location_callback) {
                            location_callback(clean_address);
                        }
                        
                        free(clean_address);
                    } else {
                        printf("Failed to extract a valid address from AI response\n");
                        if (clean_address) {
                            free(clean_address);
                        }
                    }
                } else {
                    printf("Failed to get formatted address\n");
                }
                 
                 free(location);
             } else if (!shutting_down) {
                 // Normal AI processing for non-location queries
                 printf("Getting AI response...\n");
                 char* ai_response = AI_processTranscription();
                 if (ai_response) {
                     printf("AI response: %s\n", ai_response);
                 } else {
                     printf("Failed to get AI response\n");
                 }
             }
         } else {
             printf("Auto-transcription failed or canceled\n");
         }
     }
     
     pthread_mutex_lock(&mic_mutex);
     pthread_cond_broadcast(&mic_cond);
     pthread_mutex_unlock(&mic_mutex);
     
     return NULL;
 }
 
 // Function to handle rotary button press detection
 static void *button_listener(void *arg) {
     (void)arg; 
     
     int prevPushValue = RotaryState_getPushValue();
     
     while (listener_active && !shutting_down) {
         RotaryState_doPushState();
         int currentPushValue = RotaryState_getPushValue();
         
         // Button state changed
         if (currentPushValue != prevPushValue) {
             // Button was pressed
             if (currentPushValue > prevPushValue) {
                 // Only start recording if not already recording
                 if (!Microphone_isRecording()) {
                     printf("Button pressed! Starting voice recording...\n");
                     // Start recording
                     Microphone_startRecording();
                 } else {
                     printf("Already recording - button press ignored\n");
                 }
             }
             
             prevPushValue = currentPushValue;
         }
         
         usleep(10000); // 10ms sleep to reduce CPU usage
     }
     
     return NULL;
 }
 
 // Initialize the microphone
 void Microphone_init(void) {
     pthread_mutex_lock(&mic_mutex);
     recording_active = 0;
     listener_active = 0;
     consecutive_silent_frames = 0;
     shutting_down = 0;
     pthread_mutex_unlock(&mic_mutex);
     
     // Ensure output directory exists
     char mkdir_cmd[512];
     snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", output_folder);
     system(mkdir_cmd);
     
     // Ensure rotary encoder is initialized
     RotaryState_init();
 }
 
 // Cleanup and free resources
 void Microphone_cleanup(void) {
     // Set the shutdown flag
     shutting_down = 1;
     
     // Stop recording if active
     if (recording_active) {
         Microphone_stopRecording();
     }
     
     // Stop listener if active
     if (listener_active) {
         Microphone_stopButtonListener();
     }
     
     // Kill any remaining arecord process
     if (arecord_pid > 0) {
         kill(arecord_pid, SIGTERM);
         waitpid(arecord_pid, NULL, 0);
         arecord_pid = -1;
     }
     
     // Clean up rotary state
     RotaryState_cleanup();
 }
 
 // Start recording audio
 int Microphone_startRecording(void) {
     pthread_mutex_lock(&mic_mutex);
     if (recording_active || shutting_down) {
         printf("Recording already in progress or system shutting down\n");
         pthread_mutex_unlock(&mic_mutex);
         return -1;
     }
     
     recording_active = 1;
     consecutive_silent_frames = 0;
     pthread_mutex_unlock(&mic_mutex);
     
     // Allocate memory for the duration argument (0 = manual stop)
     int *duration_arg = malloc(sizeof(int));
     if (!duration_arg) {
         perror("malloc");
         pthread_mutex_lock(&mic_mutex);
         recording_active = 0;
         pthread_mutex_unlock(&mic_mutex);
         return -1;
     }
     
     *duration_arg = 0; // 0 means manual stop
     
     // Create thread to handle recording
     if (pthread_create(&record_thread, NULL, record_audio, duration_arg) != 0) {
         perror("pthread_create");
         free(duration_arg);
         pthread_mutex_lock(&mic_mutex);
         recording_active = 0;
         pthread_mutex_unlock(&mic_mutex);
         return -1;
     }
     
     // Make thread detached so it cleans up after itself
     pthread_detach(record_thread);
     
     return 0;
 }
 
 // Stop recording audio
 int Microphone_stopRecording(void) {
     pthread_mutex_lock(&mic_mutex);
     if (!recording_active) {
         pthread_mutex_unlock(&mic_mutex);
         return -1;
     }
     
     recording_active = 0;
     pthread_cond_wait(&mic_cond, &mic_mutex);
     pthread_mutex_unlock(&mic_mutex);
     
     return 0;
 }
 
 // Check if currently recording
 bool Microphone_isRecording(void) {
     pthread_mutex_lock(&mic_mutex);
     int is_recording = recording_active;
     pthread_mutex_unlock(&mic_mutex);
     
     return is_recording != 0;
 }
 
 // Set whether to auto-transcribe when recording stops
 void Microphone_setAutoTranscribe(bool enable) {
     auto_transcribe_on_stop = enable ? 1 : 0;
 }
 
 // Function to register a callback for formatted location addresses
 void Microphone_setLocationCallback(void (*callback)(const char* location)) {
     location_callback = callback;
 }
 
 // Transcribe the last recording and return the result
 char* Microphone_transcribe(void) {
     if (shutting_down) {
         printf("Cannot transcribe while shutting down\n");
         return NULL;
     }
     
     // Clear previous result
     memset(transcription_result, 0, sizeof(transcription_result));
     
     // Prepare full audio file path
     char audio_path[512];
     snprintf(audio_path, sizeof(audio_path), "%s/%s", output_folder, output_filename);
     
     // Check if audio file exists
     if (access(audio_path, F_OK) == -1) {
         printf("Error: No audio file found at %s. Recording may have failed.\n", audio_path);
         return NULL;
     }
     
     printf("Transcribing audio from %s...\n", audio_path);
     
     // Check if the Python script exists in the current directory
     if (access("speech.py", F_OK) == -1) {
         printf("Error: speech.py not found in the current directory.\n");
         return NULL;
     }
     
     // Run the Python script with the audio file path - with more space for command
     char cmd[2048];  // Larger buffer to avoid truncation warnings
     snprintf(cmd, sizeof(cmd), "python3 ./speech.py %s", audio_path);
     
     // Use timeout version to prevent hanging
     char *output = read_command_output_with_timeout(cmd, 10); // 10 second timeout
     if (!output || shutting_down) {
         printf("Error: Failed to run speech recognition or was canceled.\n");
         free(output);
         return NULL;
     }
     
     // Prepare full transcription output path
     char transcription_path[512];
     snprintf(transcription_path, sizeof(transcription_path), "%s/%s", output_folder, transcription_filename);
     
     // Store the raw output as the transcription result
     if (output && strlen(output) > 0) {
         // Strip newlines and limit length
         char *newline = strchr(output, '\n');
         if (newline) *newline = '\0';
         
         if (strlen(output) < sizeof(transcription_result) - 1) {
             strcpy(transcription_result, output);
             
             // Save transcription to file
             FILE *transcription_file = fopen(transcription_path, "w");
             if (transcription_file) {
                 fprintf(transcription_file, "%s", transcription_result);
                 fclose(transcription_file);
                 printf("Transcription saved to %s\n", transcription_path);
             } else {
                 perror("Failed to save transcription to file");
             }
             
             free(output);
             
             // Call the callback if registered
             if (transcription_callback && !shutting_down) {
                 transcription_callback(transcription_result);
             }
             
             return transcription_result;
         }
     }
     
     free(output);
     printf("Error: Failed to parse transcription result.\n");
     return NULL;
 }
 
 // Convenience function: record for specified duration and return transcription
 char* Microphone_recordAndTranscribe(int durationMs) {
     if (shutting_down) {
         printf("Cannot record while shutting down\n");
         return NULL;
     }
     
     printf("Starting audio recording...\n");
     
     if (durationMs > 0) {
         // For timed recording
         int *duration_arg = malloc(sizeof(int));
         if (!duration_arg) {
             perror("malloc");
             return NULL;
         }
         
         *duration_arg = durationMs;
         
         pthread_mutex_lock(&mic_mutex);
         recording_active = 1;
         consecutive_silent_frames = 0;
         pthread_mutex_unlock(&mic_mutex);
         
         if (pthread_create(&record_thread, NULL, record_audio, duration_arg) != 0) {
             perror("pthread_create");
             free(duration_arg);
             pthread_mutex_lock(&mic_mutex);
             recording_active = 0;
             pthread_mutex_unlock(&mic_mutex);
             return NULL;
         }
         
         // Wait for recording to complete
         pthread_mutex_lock(&mic_mutex);
         while (recording_active && !shutting_down) {
             pthread_cond_wait(&mic_cond, &mic_mutex);
         }
         pthread_mutex_unlock(&mic_mutex);
     } else {
         // For manual stop recording
         if (Microphone_startRecording() != 0) {
             return NULL;
         }
         
         printf("Recording... Press Enter to stop.\n");
         getchar();  // Wait for user to press Enter
         
         Microphone_stopRecording();
     }
     
     // Only transcribe if not shutting down
     if (!shutting_down) {
         // Transcribe and return the result
         return Microphone_transcribe();
     }
     
     return NULL;
 }
 
 // Start the listener thread that monitors for rotary button presses
 int Microphone_startButtonListener(void) {
     pthread_mutex_lock(&mic_mutex);
     if (listener_active || shutting_down) {
         printf("Button listener already running or system shutting down\n");
         pthread_mutex_unlock(&mic_mutex);
         return -1;
     }
     
     listener_active = 1;
     pthread_mutex_unlock(&mic_mutex);
     
     // Create thread to handle button monitoring
     if (pthread_create(&button_listener_thread, NULL, button_listener, NULL) != 0) {
         perror("pthread_create");
         pthread_mutex_lock(&mic_mutex);
         listener_active = 0;
         pthread_mutex_unlock(&mic_mutex);
         return -1;
     }
     
     // Make thread detached for automatic cleanup
     pthread_detach(button_listener_thread);
     
     return 0;
 }
 
 // Stop the listener thread
 void Microphone_stopButtonListener(void) {
     pthread_mutex_lock(&mic_mutex);
     if (!listener_active) {
         pthread_mutex_unlock(&mic_mutex);
         return;
     }
     
     listener_active = 0;
     pthread_mutex_unlock(&mic_mutex);
     
     // Thread is detached, so we don't need to join it
 }
 
 // Register a callback function to be called when speech recognition completes
 void Microphone_setTranscriptionCallback(void (*callback)(const char* transcription)) {
     transcription_callback = callback;
 }
 
 // Update the sound threshold for detection
 void Microphone_setSoundThreshold(int threshold) {
     if (threshold > 0) {
         *((int*)&SOUND_THRESHOLD) = threshold;
     }
 }