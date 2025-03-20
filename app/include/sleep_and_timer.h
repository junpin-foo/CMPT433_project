/* sleep_timer_helper.h
*  Helper functions for sleeping and time difference calculation
*/

#ifndef _SLEEPTIMERHELPER_H_
#define _SLEEPTIMERHELPER_H_

#include <time.h>

// Calculate time difference in milliseconds
long time_diff_ms(struct timespec *start, struct timespec *end);

// Sleep for a given number of milliseconds
void sleepForMs(long long delayInMs);

#endif