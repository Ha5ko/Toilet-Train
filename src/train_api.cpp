/**
 * Train API Client Implementation
 * Uses Huxley2 JSON proxy for National Rail Darwin
 */

#include "train_api.h"
#include "config.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// =============================================================================
// Static Variables
// =============================================================================

static DepartureBoard outboundBoard;
static DepartureBoard inboundBoard;
static ApiStatus currentStatus = API_STATUS_OK;
static String lastError = "";
static TrainApiCallback apiCallback = nullptr;
static String apiToken = DARWIN_API_TOKEN;
static unsigned long lastUpdateTime = 0;
static bool fetchInProgress = false;

// WiFi client for HTTPS
static WiFiClientSecure client;

// =============================================================================
// Internal Functions
// =============================================================================

static void setStatus(ApiStatus status, const char* message = "") {
    currentStatus = status;
    lastError = String(message);
    if (apiCallback) {
        apiCallback(status, message);
    }
}

static int parseTimeToMinutes(const String& timeStr) {
    // Parse "HH:MM" to minutes since midnight
    if (timeStr.length() < 5) return -1;

    int colonPos = timeStr.indexOf(':');
    if (colonPos == -1) return -1;

    int hours = timeStr.substring(0, colonPos).toInt();
    int minutes = timeStr.substring(colonPos + 1).toInt();

    return hours * 60 + minutes;
}

static int calculateDelay(const String& scheduled, const String& estimated) {
    // Calculate delay in minutes
    // estimated might be "On time", a time like "14:35", or "Delayed"

    if (estimated.equalsIgnoreCase("On time") || estimated == scheduled) {
        return 0;
    }

    if (estimated.equalsIgnoreCase("Cancelled") ||
        estimated.equalsIgnoreCase("Delayed")) {
        return -1;  // Unknown delay
    }

    int schedMins = parseTimeToMinutes(scheduled);
    int estMins = parseTimeToMinutes(estimated);

    if (schedMins == -1 || estMins == -1) {
        return 0;
    }

    // Handle day rollover (e.g., scheduled 23:50, estimated 00:05)
    int delay = estMins - schedMins;
    if (delay < -720) {  // More than 12 hours negative = day rollover
        delay += 1440;   // Add 24 hours
    }

    return delay > 0 ? delay : 0;
}

static TrainStatus determineStatus(const TrainService& service) {
    if (service.isCancelled) {
        return STATUS_CANCELLED;
    }

    if (service.delayMinutes > 10) {
        return STATUS_DELAYED;  // Significant delay
    }

    if (service.delayMinutes > 0) {
        return STATUS_DELAYED;  // Minor delay
    }

    if (service.estimatedDeparture.equalsIgnoreCase("On time") ||
        service.estimatedDeparture == service.scheduledDeparture) {
        return STATUS_ON_TIME;
    }

    return STATUS_UNKNOWN;
}

static bool parseService(JsonObject serviceJson, TrainService& service) {
    // Parse scheduled times
    service.scheduledDeparture = serviceJson["std"].as<String>();
    service.estimatedDeparture = serviceJson["etd"].as<String>();

    // Some services might not have arrival times (terminates here)
    if (!serviceJson["sta"].isNull()) {
        service.scheduledArrival = serviceJson["sta"].as<String>();
    }
    if (!serviceJson["eta"].isNull()) {
        service.estimatedArrival = serviceJson["eta"].as<String>();
    }

    // Platform
    if (!serviceJson["platform"].isNull()) {
        service.platform = serviceJson["platform"].as<String>();
    } else {
        service.platform = "-";
    }

    // Operator info
    service.operatorName = serviceJson["operator"].as<String>();
    service.operatorCode = serviceJson["operatorCode"].as<String>();

    // Service ID
    service.serviceId = serviceJson["serviceID"].as<String>();

    // Check if cancelled
    service.isCancelled = service.estimatedDeparture.equalsIgnoreCase("Cancelled");

    // Cancel/delay reasons
    if (!serviceJson["cancelReason"].isNull()) {
        service.cancelReason = serviceJson["cancelReason"].as<String>();
    }
    if (!serviceJson["delayReason"].isNull()) {
        service.delayReason = serviceJson["delayReason"].as<String>();
    }

    // Calculate delay
    service.delayMinutes = calculateDelay(service.scheduledDeparture, service.estimatedDeparture);

    // Determine status
    service.status = determineStatus(service);

    // Parse calling points (subsequent stops)
    service.callingPoints.clear();
    if (!serviceJson["subsequentCallingPoints"].isNull()) {
        JsonArray callingPointLists = serviceJson["subsequentCallingPoints"]["callingPointList"];
        if (callingPointLists.size() > 0) {
            JsonArray callingPoints = callingPointLists[0]["callingPoint"];
            for (JsonObject cp : callingPoints) {
                String locationName = cp["locationName"].as<String>();
                service.callingPoints.push_back(locationName);
            }
        }
    }

    return true;
}

static bool fetchDepartures(const char* originCRS, const char* destCRS, DepartureBoard& board) {
    if (!wifi_is_connected()) {
        board.hasData = false;
        board.errorMessage = "No WiFi connection";
        return false;
    }

    // Build URL
    // Format: /departures/{crs}/to/{filterCrs}/{numRows}?accessToken={token}
    String url = String(HUXLEY_API_BASE_URL) + "/departures/" +
                 String(originCRS) + "/to/" + String(destCRS) + "/" +
                 String(NUM_SERVICES_TO_FETCH) +
                 "?accessToken=" + apiToken +
                 "&timeWindow=" + String(DEPARTURE_TIME_WINDOW);

    DEBUG_PRINTF("Fetching: %s\n", url.c_str());

    HTTPClient http;
    client.setInsecure();  // Skip certificate verification for simplicity

    http.begin(client, url);
    http.setTimeout(API_TIMEOUT_MS);
    http.addHeader("Accept", "application/json");

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTF("HTTP error: %d\n", httpCode);
        board.hasData = false;
        board.errorMessage = "HTTP error: " + String(httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DEBUG_PRINTF("Response size: %d bytes\n", payload.length());

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        DEBUG_PRINTF("JSON parse error: %s\n", error.c_str());
        board.hasData = false;
        board.errorMessage = "JSON parse error";
        return false;
    }

    // Check for API errors
    if (!doc["error"].isNull()) {
        String apiError = doc["error"].as<String>();
        DEBUG_PRINTF("API error: %s\n", apiError.c_str());
        board.hasData = false;
        board.errorMessage = apiError;
        return false;
    }

    // Parse board info
    board.originStation = doc["locationName"].as<String>();
    board.destinationStation = doc["filterLocationName"].as<String>();
    board.originCRS = String(originCRS);
    board.destinationCRS = String(destCRS);
    board.generatedAt = doc["generatedAt"].as<String>();
    board.services.clear();

    // Parse services
    if (!doc["trainServices"].isNull()) {
        JsonArray services = doc["trainServices"].as<JsonArray>();
        for (JsonObject serviceJson : services) {
            TrainService service;
            if (parseService(serviceJson, service)) {
                board.services.push_back(service);
            }
        }
    }

    board.hasData = true;
    board.errorMessage = "";

    DEBUG_PRINTF("Parsed %d services for %s -> %s\n",
                 board.services.size(), originCRS, destCRS);

    return true;
}

// =============================================================================
// Public Functions
// =============================================================================

void train_api_init() {
    DEBUG_PRINTLN("Initializing Train API client...");

    // Initialize boards
    outboundBoard.originStation = STATION_LUTON_NAME;
    outboundBoard.destinationStation = STATION_FARRINGDON_NAME;
    outboundBoard.originCRS = STATION_LUTON_CRS;
    outboundBoard.destinationCRS = STATION_FARRINGDON_CRS;
    outboundBoard.hasData = false;

    inboundBoard.originStation = STATION_FARRINGDON_NAME;
    inboundBoard.destinationStation = STATION_LUTON_NAME;
    inboundBoard.originCRS = STATION_FARRINGDON_CRS;
    inboundBoard.destinationCRS = STATION_LUTON_CRS;
    inboundBoard.hasData = false;

    currentStatus = API_STATUS_OK;
    DEBUG_PRINTLN("Train API client initialized");
}

void train_api_set_token(const char* token) {
    apiToken = String(token);
    DEBUG_PRINTF("API token set: %s\n", token);
}

void train_api_fetch_all() {
    if (fetchInProgress) {
        DEBUG_PRINTLN("Fetch already in progress, skipping");
        return;
    }

    if (!wifi_is_connected()) {
        setStatus(API_STATUS_NO_WIFI, "No WiFi connection");
        return;
    }

    fetchInProgress = true;
    setStatus(API_STATUS_LOADING, "Fetching train data...");

    // Fetch outbound (Luton -> Farringdon)
    bool outboundOk = fetchDepartures(
        STATION_LUTON_CRS,
        STATION_FARRINGDON_CRS,
        outboundBoard
    );

    // Fetch inbound (Farringdon -> Luton)
    bool inboundOk = fetchDepartures(
        STATION_FARRINGDON_CRS,
        STATION_LUTON_CRS,
        inboundBoard
    );

    fetchInProgress = false;
    lastUpdateTime = millis();

    if (outboundOk && inboundOk) {
        setStatus(API_STATUS_OK, "Data updated");
    } else if (outboundOk || inboundOk) {
        setStatus(API_STATUS_OK, "Partial data updated");
    } else {
        setStatus(API_STATUS_ERROR, "Failed to fetch data");
    }
}

void train_api_fetch_departures(const char* originCRS, const char* destCRS) {
    if (!wifi_is_connected()) {
        setStatus(API_STATUS_NO_WIFI, "No WiFi connection");
        return;
    }

    DepartureBoard tempBoard;
    fetchDepartures(originCRS, destCRS, tempBoard);
}

DepartureBoard* train_api_get_outbound() {
    return &outboundBoard;
}

DepartureBoard* train_api_get_inbound() {
    return &inboundBoard;
}

ApiStatus train_api_get_status() {
    return currentStatus;
}

const char* train_api_get_error() {
    return lastError.c_str();
}

void train_api_set_callback(TrainApiCallback callback) {
    apiCallback = callback;
}

bool train_api_is_data_fresh() {
    if (lastUpdateTime == 0) return false;
    return (millis() - lastUpdateTime) < AUTO_REFRESH_INTERVAL_MS;
}

int train_api_seconds_since_update() {
    if (lastUpdateTime == 0) return -1;
    return (millis() - lastUpdateTime) / 1000;
}

void train_api_process() {
    // Currently synchronous, but could be made async in future
}

const char* train_status_to_string(TrainStatus status) {
    switch (status) {
        case STATUS_ON_TIME:
            return "On Time";
        case STATUS_DELAYED:
            return "Delayed";
        case STATUS_CANCELLED:
            return "Cancelled";
        default:
            return "Unknown";
    }
}

uint32_t train_status_to_color(TrainStatus status) {
    switch (status) {
        case STATUS_ON_TIME:
            return COLOR_STATUS_ON_TIME;
        case STATUS_DELAYED:
            return COLOR_STATUS_DELAYED;
        case STATUS_CANCELLED:
            return COLOR_STATUS_CANCELLED;
        default:
            return COLOR_TEXT_MUTED;
    }
}
