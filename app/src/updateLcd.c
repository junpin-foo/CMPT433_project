/* updateLcd.c
* 
* This file contains the implementation of the LCD update module. 
* The module is responsible for updating the LCD display with the current beat mode, volume, and BPM.
* The module also displays the audio and accelerometer timing statistics.
*
*/

#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <stdbool.h>
#include <assert.h>
#include "pthread.h"
#include "updateLcd.h"
#include "sleep_and_timer.h"
#include "speedLimitLED.h"
#include "hal/joystick.h"
#include "hal/GPS.h"
#include "roadTracker.h"

#define DELAY_MS 2000
#define SLEEP_MS 10
#define BACKLIGHT 1023
#define INITIAL_X 5
#define INITIAL_Y 40
#define NEXTLINE_Y 40
#define FREQUENCY_X 140
#define DIPS_X 120
#define MAX_MS_X 160
#define VALUE_OFFSET 40
#define statBufferSize 64

static UWORD *s_fb;
static bool isInitialized = false;
static char speed_str[statBufferSize];
static char limit_str[statBufferSize];
static char progress_str[statBufferSize];
static char source_location_str[statBufferSize];
static char target_location_str[statBufferSize];

static pthread_t outputThread;
static bool isRunning = false;
static void* UpdateLcdThread(void* args);
void UpdateLcd_init()
{
    assert(!isInitialized);

    // Exception handling:ctrl + c
    // signal(SIGINT, Handler_1IN54_LCD);
    
    // Module Init
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
	
    // LCD Init
    DEV_Delay_ms(DELAY_MS);
	LCD_1IN54_Init(HORIZONTAL);
	LCD_1IN54_Clear(WHITE);
	LCD_SetBacklight(BACKLIGHT);

    UDOUBLE Imagesize = LCD_1IN54_HEIGHT*LCD_1IN54_WIDTH*2;
    if((s_fb = (UWORD *)malloc(Imagesize)) == NULL) {
        perror("Failed to apply for black memory");
        exit(0);
    }
    isRunning = true;
    isInitialized = true;
    pthread_create(&outputThread, NULL, &UpdateLcdThread, NULL);
}
void UpdateLcd_cleanup()
{
    assert(isInitialized);
    isRunning = false;
    pthread_join(outputThread, NULL);
    LCD_1IN54_Clear(WHITE);
    LCD_SetBacklight(0);
    // Module Exit
    free(s_fb);
    s_fb = NULL;
	DEV_ModuleExit();
    isInitialized = false;
}

static void* UpdateLcdThread(void* args) {
    (void) args;
    assert(isInitialized);
    while (isRunning) {
        if (Joystick_getPageCount() == 1) {
            UpdateLcd_Speed(SpeedLED_getSpeed(), SpeedLED_getSpeedLimit());
        } else {
            UpdateLcd_roadTracker(RoadTracker_getProgress(), RoadTracker_getTargetAddress(), RoadTracker_getSourceLocation(), RoadTracker_getTargetLocation());
        }
        sleepForMs(SLEEP_MS);
    }
    return NULL;
}

void UpdateLcd_Speed(double gps_speed_kmh, int speed_limit)
{
    assert(isInitialized);

    const int x = INITIAL_X;
    int y = INITIAL_Y;

    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    sprintf(speed_str, "%.1f km/h", gps_speed_kmh);
    sprintf(limit_str, "%d km/h", speed_limit);
    Paint_DrawString_EN(x, y, "Speed:", &Font20, WHITE, BLACK);
    Paint_DrawString_EN(x + 80, y, speed_str, &Font20, WHITE, BLACK);
    y += NEXTLINE_Y;

    Paint_DrawString_EN(x, y, "Limit:", &Font20, WHITE, BLACK);
    Paint_DrawString_EN(x + 80, y, limit_str, &Font20, WHITE, BLACK);
    y += NEXTLINE_Y;

    if (gps_speed_kmh - speed_limit >= 10 || gps_speed_kmh - speed_limit <= -10) {
            Paint_DrawRectangle(85, y, 130, y + 50, YELLOW, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        } else if (gps_speed_kmh > speed_limit) {
            Paint_DrawRectangle(85, y, 130, y + 50, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        } else {
            Paint_DrawRectangle(85, y, 130, y + 50, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }

    LCD_1IN54_Display(s_fb);
}

void UpdateLcd_roadTracker(double progress, const char* target_address, struct location source_location, struct location target_location)
{
    assert(isInitialized);

    const int x = INITIAL_X;
    int y = INITIAL_Y;
    // progress = 50;
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);
    // printf("progress: %.1f", progress);
    sprintf(progress_str, "%.1f%%", progress);
    
    if (source_location.latitude == -1000 || source_location.longitude == -1000) {
        sprintf(source_location_str, "NULL");
    } else {
        snprintf(source_location_str, statBufferSize, "%.6f,%.6f", source_location.latitude, source_location.longitude);
    }

    if (target_location.latitude == -1000 || target_location.longitude == -1000) {
        sprintf(target_location_str, "NULL");
    } else {
        snprintf(target_location_str, statBufferSize, "%.6f,%.6f", target_location.latitude, target_location.longitude);
    }

    Paint_DrawString_EN(x, y, "Target Location:", &Font16, WHITE, BLACK);
    y += 20;
    Paint_DrawString_EN(x, y, target_address, &Font16, WHITE, BLACK);
    y += 40;

    Paint_DrawString_EN(x, y, "Current Location:", &Font16, WHITE, BLACK);
    y += 20;
    Paint_DrawString_EN(x, y, source_location_str, &Font16, WHITE, BLACK);
    y += 20;

    Paint_DrawString_EN(x, y, "Progress:", &Font16, WHITE, BLACK);
    Paint_DrawString_EN(x + 130, y, progress_str, &Font16, WHITE, BLACK);
    y += 20;

    if (progress < 100) {
        // Progress Bar Drawing
        int progressBarWidth = LCD_1IN54_WIDTH;  // Width of the progress bar (can be adjusted)
        int progressBarHeight = 30;  // Height of the progress bar
        int progressBarX = x;        // X position for the progress bar
        int progressBarY = y + 10;   // Y position for the progress bar (slightly below the text)

        // Calculate the width of the filled progress bar based on the progress
        int filledWidth = (int)((progress / 100.0) * progressBarWidth);
        // // Draw the progress bar (empty part)
        // Paint_DrawRectangle(progressBarX, progressBarY, progressBarX + progressBarWidth, progressBarY + progressBarHeight, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        
        // Draw the filled part of the progress bar
        Paint_DrawRectangle(progressBarX, progressBarY, progressBarX + filledWidth, progressBarY + progressBarHeight, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    LCD_1IN54_Display(s_fb);
}
