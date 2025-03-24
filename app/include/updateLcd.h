/* updateLcd.h
 * 
 * This module handles the LCD screen output, it start a thread and repeately display periodic statistics.
 * 
 */
#ifndef _UPDATELCD_H_
#define _UPDATELCD_H_

//Initialize and clean up the LCD screen.
void UpdateLcd_init();
void UpdateLcd_cleanup();
void UpdateLcd(double gps_speed_kmh, int speed_limit);

#endif