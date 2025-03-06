/* joystick.h
    This header file defines the interface for joystick control using an ADC chip over I2C.
    It provides functions to initialize and clean up the joystick, as well as retrieve 
    the current joystick position with normalized x and y values.
*/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define LED_FILE_NAME "/sys/class/leds"

struct JoystickData {
    double x;
    double y;
    bool isPressed;
};

void Joystick_initialize(void);
void Joystick_cleanUp(void);
/*
This function reads the current joystick position and returns the x and y values after scaling.
    @Return: JoystickData struct containing the x and y values scaled between (-1 and 1) and isPressed.
*/
struct JoystickData Joystick_getReading();

#endif