// parking.h
#ifndef PARKING_H
#define PARKING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

void Parking_init();
void Parking_cleanup();
bool Parking_Activate();

int Parking_getMode(void);

int Parking_getColor(void);


#endif // PARKING_H