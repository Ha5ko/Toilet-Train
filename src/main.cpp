/**
 * UK Train Departure Display for ESP32-4848S040
 *
 * Shows live train departures between Luton and Farringdon
 * using the National Rail Darwin API via Huxley2 proxy.
 *
 * Hardware: ESP32-S3 with 4" 480x480 ST7701 TFT + GT911 Touch
 */

#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "wifi_manager.h"
#include "train_api.h"
#include "ui.h"

// =============================================================================
// Application State
// =============================================================================

enum AppState {
    STATE_INIT,
    STATE_WIFI_CONNECTING,
    STATE_WIFI_PORTAL,
    STATE_FETCHING_DATA,
    STATE_RUNNING,
    STATE_ERROR
};

static AppState currentState = STATE_INIT;
static unsigned long lastRefreshTime = 0;
static unsigned long lastDisplayUpdate = 0;
static bool initialFetchDone = false;

// =============================================================================
// Callbacks
// =============================================================================

// WiFi status callback
static void onWiFiStatus(WiFiStatus status, const char* message) {
    DEBUG_PRINTF("WiFi status: %d - %s\n", status, message);

    switch (status) {
        case WIFI_STATUS_CONNECTING:
            ui_update_status("Connecting to WiFi...");
            break;
        case WIFI_STATUS_CONNECTED:
            ui_update_status("WiFi connected!");
            ui_update_wifi_status(true, wifi_get_rssi());
            break;
        case WIFI_STATUS_PORTAL_ACTIVE:
            currentState = STATE_WIFI_PORTAL;
            ui_show_wifi_setup(WIFI_AP_NAME, "192.168.4.1");
            break;
        case WIFI_STATUS_FAILED:
            ui_update_status("WiFi connection failed");
            break;
        default:
            break;
    }
}

// Train API status callback
static void onTrainApiStatus(ApiStatus status, const char* message) {
    DEBUG_PRINTF("API status: %d - %s\n", status, message);

    switch (status) {
        case API_STATUS_LOADING:
            ui_show_loading("Fetching train data...");
            break;
        case API_STATUS_OK:
            ui_hide_loading();
            ui_update_status("Data updated");
            ui_update_train_data();
            break;
        case API_STATUS_ERROR:
            ui_hide_loading();
            ui_update_status(message);
            break;
        case API_STATUS_NO_WIFI:
            ui_hide_loading();
            ui_update_status("No WiFi - reconnecting...");
            break;
    }
}

// Refresh button callback
static void onRefreshPressed(lv_event_t* e) {
    DEBUG_PRINTLN("Refresh button pressed");

    // Check minimum interval between refreshes
    if (millis() - lastRefreshTime < MIN_REFRESH_INTERVAL_MS) {
        DEBUG_PRINTLN("Refresh too soon, ignoring");
        ui_update_status("Please wait...");
        return;
    }

    lastRefreshTime = millis();
    train_api_fetch_all();
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(1000);  // Give serial time to initialize

    DEBUG_PRINTLN("\n\n================================");
    DEBUG_PRINTLN("UK Train Departure Display");
    DEBUG_PRINTLN("ESP32-4848S040");
    DEBUG_PRINTLN("================================\n");

    // Initialize display and LVGL
    DEBUG_PRINTLN("Initializing display...");
    if (!display_init()) {
        DEBUG_PRINTLN("Display initialization failed!");
        // Can't show error on display, just halt
        while (1) {
            delay(1000);
        }
    }

    // Initialize UI
    DEBUG_PRINTLN("Initializing UI...");
    ui_init();

    // Show splash screen
    ui_show_screen(SCREEN_SPLASH);
    display_update();
    delay(100);  // Let splash render

    // Initialize WiFi manager
    DEBUG_PRINTLN("Initializing WiFi manager...");
    wifi_init();
    wifi_set_status_callback(onWiFiStatus);

    // Initialize train API
    DEBUG_PRINTLN("Initializing Train API...");
    train_api_init();
    train_api_set_callback(onTrainApiStatus);

    // Set API token from WiFi manager (may be stored in preferences)
    train_api_set_token(wifi_get_api_token());

    // Register refresh button callback
    lv_obj_t* btn = ui_get_refresh_button();
    if (btn) {
        lv_obj_add_event_cb(btn, onRefreshPressed, LV_EVENT_CLICKED, nullptr);
    }

    // Update display a few times during init
    for (int i = 0; i < 10; i++) {
        display_update();
        delay(50);
    }

    currentState = STATE_WIFI_CONNECTING;
    DEBUG_PRINTLN("Setup complete, connecting to WiFi...");
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
    // Update display (LVGL)
    display_update();

    // State machine
    switch (currentState) {
        case STATE_INIT:
            // Should not reach here after setup
            currentState = STATE_WIFI_CONNECTING;
            break;

        case STATE_WIFI_CONNECTING:
            // Attempt WiFi connection
            ui_update_status("Connecting to WiFi...");
            if (wifi_connect()) {
                // Connected successfully
                DEBUG_PRINTLN("WiFi connected!");
                ui_show_screen(SCREEN_MAIN);
                ui_update_wifi_status(true, wifi_get_rssi());
                currentState = STATE_FETCHING_DATA;

                // Update API token in case it changed
                train_api_set_token(wifi_get_api_token());
            } else {
                // Portal should be active now
                DEBUG_PRINTLN("WiFi portal active");
                currentState = STATE_WIFI_PORTAL;
            }
            break;

        case STATE_WIFI_PORTAL:
            // Process WiFi manager portal
            wifi_process();

            // Check if we got connected
            if (wifi_is_connected()) {
                DEBUG_PRINTLN("WiFi connected via portal!");
                ui_show_screen(SCREEN_MAIN);
                ui_update_wifi_status(true, wifi_get_rssi());
                currentState = STATE_FETCHING_DATA;

                // Update API token
                train_api_set_token(wifi_get_api_token());
            }
            break;

        case STATE_FETCHING_DATA:
            // Initial data fetch
            if (!initialFetchDone) {
                DEBUG_PRINTLN("Fetching initial train data...");
                train_api_fetch_all();
                initialFetchDone = true;
                lastRefreshTime = millis();
            }
            currentState = STATE_RUNNING;
            break;

        case STATE_RUNNING:
            // Normal operation

            // Check WiFi connection
            if (!wifi_is_connected()) {
                DEBUG_PRINTLN("WiFi disconnected, reconnecting...");
                ui_update_wifi_status(false, 0);
                ui_update_status("WiFi disconnected...");
                currentState = STATE_WIFI_CONNECTING;
                break;
            }

            // Update WiFi status periodically
            static unsigned long lastWifiCheck = 0;
            if (millis() - lastWifiCheck > 5000) {
                ui_update_wifi_status(true, wifi_get_rssi());
                lastWifiCheck = millis();
            }

            // Auto-refresh data
            if (millis() - lastRefreshTime >= AUTO_REFRESH_INTERVAL_MS) {
                DEBUG_PRINTLN("Auto-refreshing data...");
                train_api_fetch_all();
                lastRefreshTime = millis();
            }

            // Update UI periodically (refresh time display, etc.)
            if (millis() - lastDisplayUpdate > 1000) {
                ui_process();
                lastDisplayUpdate = millis();
            }

            // Process any pending API requests
            train_api_process();
            break;

        case STATE_ERROR:
            // Error state - could add retry logic here
            delay(1000);
            break;
    }

    // Small delay to prevent tight loop
    delay(5);
}
