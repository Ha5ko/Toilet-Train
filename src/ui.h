/**
 * UI Components for Train Display
 * LVGL-based user interface
 */

#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <lvgl.h>
#include "train_api.h"

// UI Screens
enum UIScreen {
    SCREEN_SPLASH,
    SCREEN_WIFI_SETUP,
    SCREEN_MAIN,
    SCREEN_ERROR
};

// Initialize the UI
void ui_init();

// Show specific screen
void ui_show_screen(UIScreen screen);

// Update the main display with train data
void ui_update_train_data();

// Show loading indicator
void ui_show_loading(const char* message);

// Hide loading indicator
void ui_hide_loading();

// Show error message
void ui_show_error(const char* title, const char* message);

// Show WiFi setup screen
void ui_show_wifi_setup(const char* ssid, const char* ip);

// Update status bar
void ui_update_status(const char* message);

// Update WiFi indicator
void ui_update_wifi_status(bool connected, int rssi);

// Update last refresh time
void ui_update_refresh_time(int secondsAgo);

// Get the refresh button (for event handling)
lv_obj_t* ui_get_refresh_button();

// Process UI updates (call in main loop)
void ui_process();

#endif // UI_H
