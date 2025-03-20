/* sleep_timer_helper.c
 *
*/

#include <stdio.h>	
#include <stdlib.h>	
#include <signal.h>     
#include <stdbool.h>
#include <assert.h>
#include <time.h>


long time_diff_ms(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000 + (end->tv_nsec - start->tv_nsec) / 1000000;
}

void sleepForMs(long long delayInMs) { 
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000; 
    long long delayNs = delayInMs * NS_PER_MS;  
    int seconds = delayNs / NS_PER_SECOND;  
    int nanoseconds = delayNs % NS_PER_SECOND;  
    struct timespec reqDelay = {seconds, nanoseconds}; 
    nanosleep(&reqDelay, (struct timespec *) NULL); 
}

