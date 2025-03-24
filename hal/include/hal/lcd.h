/* lcd.h
    This header file defines the interface for the LCD module.
    It provides functions to initialize and clean up the LCD module.
*/

#ifndef _LCD_H_
#define _LCD_H_

// Initializes the LCD module and starts thread to query correct page to display
void Lcd_init();

// Cleans up the LCD module
void Lcd_cleanup();

#endif