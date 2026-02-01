/**
 * Configuration for UK Train Display
 * ESP32-4848S040 Train Departure Board
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// API CONFIGURATION
// =============================================================================

// Huxley2 API endpoint (JSON proxy for National Rail Darwin)
// Public community instance - consider self-hosting for production
#define HUXLEY_API_BASE_URL "https://huxley2.azurewebsites.net"

// Your National Rail Darwin API token
// Get one free at: https://www.nationalrail.co.uk/developers/
// Leave as placeholder until you receive your token
#define DARWIN_API_TOKEN "YOUR_API_TOKEN_HERE"

// =============================================================================
// STATION CONFIGURATION
// =============================================================================

// Station CRS codes - Computer Reservation System codes
// Luton station
#define STATION_LUTON_CRS "LUT"
#define STATION_LUTON_NAME "Luton"

// Farringdon station
#define STATION_FARRINGDON_CRS "ZFD"
#define STATION_FARRINGDON_NAME "Farringdon"

// Time window for departures (in minutes, max 120)
#define DEPARTURE_TIME_WINDOW 120

// Number of services to fetch per direction
#define NUM_SERVICES_TO_FETCH 10

// =============================================================================
// REFRESH SETTINGS
// =============================================================================

// Auto-refresh interval in milliseconds (60 seconds)
#define AUTO_REFRESH_INTERVAL_MS 60000

// Minimum time between manual refreshes (prevent spamming)
#define MIN_REFRESH_INTERVAL_MS 10000

// API request timeout in milliseconds
#define API_TIMEOUT_MS 15000

// =============================================================================
// DISPLAY SETTINGS
// =============================================================================

// Screen dimensions
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

// Backlight PWM settings
#define BACKLIGHT_PIN 38
#define BACKLIGHT_CHANNEL 0
#define BACKLIGHT_FREQ 5000
#define BACKLIGHT_RESOLUTION 8
#define BACKLIGHT_DEFAULT_BRIGHTNESS 200  // 0-255

// =============================================================================
// COLOR SCHEME (Dark theme)
// =============================================================================

// Background colors
#define COLOR_BG_DARK         0x1a1a2e  // Deep navy background
#define COLOR_BG_PANEL        0x16213e  // Panel background
#define COLOR_BG_CARD         0x0f3460  // Card background

// Text colors
#define COLOR_TEXT_PRIMARY    0xeaeaea  // Main text
#define COLOR_TEXT_SECONDARY  0x94a3b8  // Secondary text
#define COLOR_TEXT_MUTED      0x64748b  // Muted text

// Status colors
#define COLOR_STATUS_ON_TIME  0x22c55e  // Green - on time
#define COLOR_STATUS_DELAYED  0xf59e0b  // Amber - delayed
#define COLOR_STATUS_LATE     0xef4444  // Red - very late (>10 min)
#define COLOR_STATUS_CANCELLED 0xdc2626 // Dark red - cancelled

// Accent colors
#define COLOR_ACCENT_BLUE     0x3b82f6  // Blue accent
#define COLOR_ACCENT_PURPLE   0x8b5cf6  // Purple accent
#define COLOR_HEADER_OUTBOUND 0x0ea5e9  // Cyan for outbound header
#define COLOR_HEADER_INBOUND  0xa855f7  // Purple for inbound header

// =============================================================================
// WIFI MANAGER SETTINGS
// =============================================================================

// Access point name when in setup mode
#define WIFI_AP_NAME "TrainDisplay-Setup"

// Access point password (min 8 characters, or empty for open)
#define WIFI_AP_PASSWORD "traintime"

// Portal timeout in seconds (0 = no timeout)
#define WIFI_PORTAL_TIMEOUT 180

// =============================================================================
// NTP TIME SETTINGS
// =============================================================================

// NTP server
#define NTP_SERVER "pool.ntp.org"

// UK timezone offset (GMT = 0, BST = 1)
// Note: Will need manual adjustment or DST logic
#define UTC_OFFSET_HOURS 0

// =============================================================================
// DEBUG SETTINGS
// =============================================================================

// Enable serial debug output
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(fmt, ...)
#endif

#endif // CONFIG_H
