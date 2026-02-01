/**
 * WiFi Manager Implementation with Captive Portal
 */

#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>

// Preferences for storing API token
static Preferences preferences;
static String apiToken = "";
static WiFiStatus currentStatus = WIFI_STATUS_DISCONNECTED;
static WiFiStatusCallback statusCallback = nullptr;
static WiFiManager wifiManager;
static bool portalActive = false;

// Custom parameter for API token
static WiFiManagerParameter* apiTokenParam = nullptr;

// =============================================================================
// Internal Functions
// =============================================================================

static void setStatus(WiFiStatus status, const char* message = "") {
    currentStatus = status;
    if (statusCallback) {
        statusCallback(status, message);
    }
}

static void saveApiToken(const char* token) {
    preferences.begin("train-display", false);
    preferences.putString("api_token", token);
    preferences.end();
    apiToken = String(token);
    DEBUG_PRINTF("API token saved: %s\n", token);
}

static void loadApiToken() {
    preferences.begin("train-display", true);
    apiToken = preferences.getString("api_token", DARWIN_API_TOKEN);
    preferences.end();
    DEBUG_PRINTF("API token loaded: %s\n", apiToken.c_str());
}

// Callback when config mode is entered
static void configModeCallback(WiFiManager* myWiFiManager) {
    DEBUG_PRINTLN("Entered config mode");
    DEBUG_PRINT("AP IP: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
    DEBUG_PRINT("Portal SSID: ");
    DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());

    setStatus(WIFI_STATUS_PORTAL_ACTIVE, "Connect to WiFi: " WIFI_AP_NAME);
    portalActive = true;
}

// Callback when settings are saved
static void saveConfigCallback() {
    DEBUG_PRINTLN("Config saved");

    // Save the API token
    if (apiTokenParam) {
        const char* token = apiTokenParam->getValue();
        if (token && strlen(token) > 0) {
            saveApiToken(token);
        }
    }
}

// =============================================================================
// Public Functions
// =============================================================================

void wifi_init() {
    DEBUG_PRINTLN("Initializing WiFi manager...");

    // Load saved API token
    loadApiToken();

    // Configure WiFiManager
    wifiManager.setDebugOutput(DEBUG_ENABLED);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
    wifiManager.setConnectTimeout(30);

    // Set dark theme for portal
    wifiManager.setClass("invert");

    // Custom HTML for portal header
    wifiManager.setCustomHeadElement(
        "<style>"
        "body { background: #1a1a2e; color: #eaeaea; }"
        ".c { text-align: center; }"
        "h1 { color: #3b82f6; }"
        "input, select { background: #16213e; color: #eaeaea; border: 1px solid #3b82f6; }"
        "button { background: #3b82f6; color: white; border: none; }"
        "</style>"
    );

    // Add custom parameter for API token
    apiTokenParam = new WiFiManagerParameter(
        "api_token",                    // ID
        "National Rail API Token",       // Label
        apiToken.c_str(),               // Default value
        64                              // Max length
    );
    wifiManager.addParameter(apiTokenParam);

    // Add info text
    WiFiManagerParameter infoText(
        "<p style='font-size:12px;color:#94a3b8;'>"
        "Get your free API token from:<br>"
        "<a href='https://www.nationalrail.co.uk/developers/' style='color:#3b82f6;'>"
        "nationalrail.co.uk/developers</a>"
        "</p>"
    );
    wifiManager.addParameter(&infoText);

    DEBUG_PRINTLN("WiFi manager initialized");
}

bool wifi_connect() {
    DEBUG_PRINTLN("Attempting WiFi connection...");
    setStatus(WIFI_STATUS_CONNECTING, "Connecting to WiFi...");

    // Try to connect with saved credentials
    // If fails, will automatically start config portal
    bool connected = wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD);

    if (connected) {
        DEBUG_PRINTLN("WiFi connected!");
        DEBUG_PRINT("IP: ");
        DEBUG_PRINTLN(WiFi.localIP());
        setStatus(WIFI_STATUS_CONNECTED, "Connected!");
        portalActive = false;

        // Update API token from parameter if changed
        if (apiTokenParam) {
            const char* token = apiTokenParam->getValue();
            if (token && strlen(token) > 0 && String(token) != apiToken) {
                saveApiToken(token);
            }
        }

        return true;
    } else {
        DEBUG_PRINTLN("WiFi connection failed");
        setStatus(WIFI_STATUS_FAILED, "Connection failed");
        return false;
    }
}

void wifi_start_portal() {
    DEBUG_PRINTLN("Starting config portal...");
    setStatus(WIFI_STATUS_PORTAL_ACTIVE, "Config portal active");
    portalActive = true;

    wifiManager.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASSWORD);

    if (WiFi.status() == WL_CONNECTED) {
        setStatus(WIFI_STATUS_CONNECTED, "Connected!");
        portalActive = false;
    }
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

WiFiStatus wifi_get_status() {
    // Update status based on actual WiFi state
    if (portalActive) {
        return WIFI_STATUS_PORTAL_ACTIVE;
    }

    switch (WiFi.status()) {
        case WL_CONNECTED:
            return WIFI_STATUS_CONNECTED;
        case WL_DISCONNECTED:
        case WL_CONNECTION_LOST:
            return WIFI_STATUS_DISCONNECTED;
        case WL_CONNECT_FAILED:
        case WL_NO_SSID_AVAIL:
            return WIFI_STATUS_FAILED;
        default:
            return WIFI_STATUS_CONNECTING;
    }
}

const char* wifi_get_api_token() {
    return apiToken.c_str();
}

void wifi_set_status_callback(WiFiStatusCallback callback) {
    statusCallback = callback;
}

void wifi_process() {
    if (portalActive) {
        wifiManager.process();
    }
}

void wifi_reset() {
    DEBUG_PRINTLN("Resetting WiFi credentials...");
    wifiManager.resetSettings();

    // Also clear API token
    preferences.begin("train-display", false);
    preferences.clear();
    preferences.end();
    apiToken = DARWIN_API_TOKEN;

    setStatus(WIFI_STATUS_DISCONNECTED, "Credentials reset");
}

String wifi_get_ip() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

int wifi_get_rssi() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}
