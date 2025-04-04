// speed_limit.h
#ifndef SPEED_LED_H
#define SPEED_LED_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void SpeedLED_init(void);

void SpeedLED_cleanup(void);

int SpeedLED_getSpeedLimit(void);

double SpeedLED_getSpeed(void);

int SpeedLED_getLEDColor(void);

#endif