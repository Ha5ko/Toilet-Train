/**
 * Display Driver for ESP32-4848S040
 * 4" 480x480 ST7701 RGB LCD with GT911 Touch
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <lvgl.h>

// Initialize the display hardware and LVGL
bool display_init();

// Update LVGL (call in main loop)
void display_update();

// Set backlight brightness (0-255)
void display_set_brightness(uint8_t brightness);

// Get touch coordinates (returns true if touched)
bool display_get_touch(uint16_t* x, uint16_t* y);

#endif // DISPLAY_H
