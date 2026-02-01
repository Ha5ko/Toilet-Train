/**
 * WiFi Manager with Captive Portal
 * For initial WiFi setup and API token configuration
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// WiFi connection status
enum WiFiStatus {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_PORTAL_ACTIVE,
    WIFI_STATUS_FAILED
};

// Callback type for status updates
typedef void (*WiFiStatusCallback)(WiFiStatus status, const char* message);

// Initialize WiFi manager
void wifi_init();

// Attempt to connect with saved credentials, or start portal if none
// Returns true if connected, false if portal was started
bool wifi_connect();

// Force start the configuration portal
void wifi_start_portal();

// Check if connected to WiFi
bool wifi_is_connected();

// Get current WiFi status
WiFiStatus wifi_get_status();

// Get the configured API token
const char* wifi_get_api_token();

// Set status callback
void wifi_set_status_callback(WiFiStatusCallback callback);

// Process WiFi manager (call in loop when portal is active)
void wifi_process();

// Reset saved credentials
void wifi_reset();

// Get IP address as string
String wifi_get_ip();

// Get signal strength
int wifi_get_rssi();

#endif // WIFI_MANAGER_H
