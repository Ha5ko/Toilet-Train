/**
 * Train API Client for Huxley2 / National Rail Darwin
 * Fetches live departure information
 */

#ifndef TRAIN_API_H
#define TRAIN_API_H

#include <Arduino.h>
#include <vector>

// Maximum services to store per direction
#define MAX_SERVICES 10

// =============================================================================
// Data Structures
// =============================================================================

// Train service status
enum TrainStatus {
    STATUS_ON_TIME,
    STATUS_DELAYED,
    STATUS_CANCELLED,
    STATUS_UNKNOWN
};

// Individual train service
struct TrainService {
    String scheduledDeparture;  // "14:30"
    String estimatedDeparture;  // "14:35" or "On time" or "Cancelled"
    String scheduledArrival;    // "15:15"
    String estimatedArrival;    // "15:20" or "On time"
    String platform;            // "1" or empty
    String operatorName;        // "Thameslink"
    String operatorCode;        // "TL"
    TrainStatus status;         // Computed status
    int delayMinutes;          // Delay in minutes (0 if on time)
    String cancelReason;        // Reason for cancellation if cancelled
    String delayReason;         // Reason for delay if delayed
    String serviceId;           // Service ID for detail lookup
    bool isCancelled;
    std::vector<String> callingPoints;  // List of stops
};

// Departure board for a direction
struct DepartureBoard {
    String originStation;
    String destinationStation;
    String originCRS;
    String destinationCRS;
    String generatedAt;         // Time data was generated
    std::vector<TrainService> services;
    bool hasData;
    String errorMessage;
};

// API status
enum ApiStatus {
    API_STATUS_OK,
    API_STATUS_LOADING,
    API_STATUS_ERROR,
    API_STATUS_NO_WIFI
};

// Callback for API updates
typedef void (*TrainApiCallback)(ApiStatus status, const char* message);

// =============================================================================
// Public Functions
// =============================================================================

// Initialize the train API client
void train_api_init();

// Set the API token
void train_api_set_token(const char* token);

// Fetch departures for both directions
// This is asynchronous - use callback to get notified
void train_api_fetch_all();

// Fetch departures from origin to destination
void train_api_fetch_departures(const char* originCRS, const char* destCRS);

// Get the outbound departure board (Luton -> Farringdon)
DepartureBoard* train_api_get_outbound();

// Get the inbound departure board (Farringdon -> Luton)
DepartureBoard* train_api_get_inbound();

// Get current API status
ApiStatus train_api_get_status();

// Get last error message
const char* train_api_get_error();

// Set callback for API updates
void train_api_set_callback(TrainApiCallback callback);

// Check if data is fresh (within refresh interval)
bool train_api_is_data_fresh();

// Get time since last update in seconds
int train_api_seconds_since_update();

// Process API requests (call in main loop)
void train_api_process();

// Get human-readable status string
const char* train_status_to_string(TrainStatus status);

// Get status color (returns hex color code)
uint32_t train_status_to_color(TrainStatus status);

#endif // TRAIN_API_H
