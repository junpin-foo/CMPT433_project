/* updateLcd.h
 * 
 * This module handles the LCD screen output, it start a thread and repeately display periodic statistics.
 * 
 */
#ifndef _UPDATELCD_H_
#define _UPDATELCD_H_
#include "hal/GPS.h"

//Initialize and clean up the LCD screen.
void UpdateLcd_init();
void UpdateLcd_cleanup();
void UpdateLcd_Speed(double gps_speed_kmh, int speed_limit);
void UpdateLcd_Speed(double gps_speed_kmh, int speed_limit);
void UpdateLcd_roadTracker(double progress, const char* target_address, struct location source_location, struct location target_location);

#endif