/**
 * UI Components Implementation
 * LVGL-based train departure display
 */

#include "ui.h"
#include "config.h"
#include "train_api.h"

// =============================================================================
// LVGL Color Helpers
// =============================================================================

static lv_color_t hex_to_lv_color(uint32_t hex) {
    return lv_color_make((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

// =============================================================================
// UI Elements
// =============================================================================

// Screens
static lv_obj_t* screen_splash = nullptr;
static lv_obj_t* screen_wifi = nullptr;
static lv_obj_t* screen_main = nullptr;
static lv_obj_t* screen_error = nullptr;

// Main screen elements
static lv_obj_t* panel_outbound = nullptr;
static lv_obj_t* panel_inbound = nullptr;
static lv_obj_t* status_bar = nullptr;
static lv_obj_t* label_status = nullptr;
static lv_obj_t* label_wifi = nullptr;
static lv_obj_t* label_time = nullptr;
static lv_obj_t* btn_refresh = nullptr;
static lv_obj_t* spinner_loading = nullptr;

// WiFi screen elements
static lv_obj_t* label_wifi_ssid = nullptr;
static lv_obj_t* label_wifi_ip = nullptr;

// Error screen elements
static lv_obj_t* label_error_title = nullptr;
static lv_obj_t* label_error_msg = nullptr;

// Service list containers
static lv_obj_t* list_outbound = nullptr;
static lv_obj_t* list_inbound = nullptr;

// Styles
static lv_style_t style_bg;
static lv_style_t style_panel;
static lv_style_t style_header_outbound;
static lv_style_t style_header_inbound;
static lv_style_t style_service_row;
static lv_style_t style_status_bar;
static lv_style_t style_btn;
static bool styles_initialized = false;

// =============================================================================
// Style Initialization
// =============================================================================

static void init_styles() {
    if (styles_initialized) return;

    // Background style
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, hex_to_lv_color(COLOR_BG_DARK));
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);

    // Panel style
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, hex_to_lv_color(COLOR_BG_PANEL));
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_radius(&style_panel, 8);
    lv_style_set_pad_all(&style_panel, 8);
    lv_style_set_border_width(&style_panel, 1);
    lv_style_set_border_color(&style_panel, hex_to_lv_color(COLOR_BG_CARD));

    // Outbound header style
    lv_style_init(&style_header_outbound);
    lv_style_set_bg_color(&style_header_outbound, hex_to_lv_color(COLOR_HEADER_OUTBOUND));
    lv_style_set_bg_opa(&style_header_outbound, LV_OPA_COVER);
    lv_style_set_radius(&style_header_outbound, 6);
    lv_style_set_pad_all(&style_header_outbound, 6);

    // Inbound header style
    lv_style_init(&style_header_inbound);
    lv_style_set_bg_color(&style_header_inbound, hex_to_lv_color(COLOR_HEADER_INBOUND));
    lv_style_set_bg_opa(&style_header_inbound, LV_OPA_COVER);
    lv_style_set_radius(&style_header_inbound, 6);
    lv_style_set_pad_all(&style_header_inbound, 6);

    // Service row style
    lv_style_init(&style_service_row);
    lv_style_set_bg_color(&style_service_row, hex_to_lv_color(COLOR_BG_CARD));
    lv_style_set_bg_opa(&style_service_row, LV_OPA_COVER);
    lv_style_set_radius(&style_service_row, 4);
    lv_style_set_pad_all(&style_service_row, 4);
    lv_style_set_pad_row(&style_service_row, 2);

    // Status bar style
    lv_style_init(&style_status_bar);
    lv_style_set_bg_color(&style_status_bar, hex_to_lv_color(0x0f0f1a));
    lv_style_set_bg_opa(&style_status_bar, LV_OPA_COVER);
    lv_style_set_pad_all(&style_status_bar, 4);

    // Button style
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, hex_to_lv_color(COLOR_ACCENT_BLUE));
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_radius(&style_btn, 4);
    lv_style_set_pad_all(&style_btn, 8);

    styles_initialized = true;
}

// =============================================================================
// Screen Creators
// =============================================================================

static void create_splash_screen() {
    screen_splash = lv_obj_create(nullptr);
    lv_obj_add_style(screen_splash, &style_bg, 0);

    // Title
    lv_obj_t* title = lv_label_create(screen_splash);
    lv_label_set_text(title, "Train Display");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, hex_to_lv_color(COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

    // Subtitle
    lv_obj_t* subtitle = lv_label_create(screen_splash);
    lv_label_set_text(subtitle, "Luton <-> Farringdon");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(subtitle, hex_to_lv_color(COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 10);

    // Loading spinner
    lv_obj_t* spinner = lv_spinner_create(screen_splash, 1000, 60);
    lv_obj_set_size(spinner, 50, 50);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 70);

    // Status
    lv_obj_t* status = lv_label_create(screen_splash);
    lv_label_set_text(status, "Initializing...");
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status, hex_to_lv_color(COLOR_TEXT_MUTED), 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 130);
}

static void create_wifi_screen() {
    screen_wifi = lv_obj_create(nullptr);
    lv_obj_add_style(screen_wifi, &style_bg, 0);

    // WiFi icon placeholder
    lv_obj_t* icon = lv_label_create(screen_wifi);
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, hex_to_lv_color(COLOR_ACCENT_BLUE), 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -80);

    // Title
    lv_obj_t* title = lv_label_create(screen_wifi);
    lv_label_set_text(title, "WiFi Setup Required");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, hex_to_lv_color(COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -20);

    // Instructions
    lv_obj_t* instr = lv_label_create(screen_wifi);
    lv_label_set_text(instr, "Connect to WiFi network:");
    lv_obj_set_style_text_font(instr, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(instr, hex_to_lv_color(COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(instr, LV_ALIGN_CENTER, 0, 30);

    // SSID label
    label_wifi_ssid = lv_label_create(screen_wifi);
    lv_label_set_text(label_wifi_ssid, WIFI_AP_NAME);
    lv_obj_set_style_text_font(label_wifi_ssid, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(label_wifi_ssid, hex_to_lv_color(COLOR_ACCENT_BLUE), 0);
    lv_obj_align(label_wifi_ssid, LV_ALIGN_CENTER, 0, 65);

    // Password info
    lv_obj_t* pwd = lv_label_create(screen_wifi);
    lv_label_set_text(pwd, "Password: " WIFI_AP_PASSWORD);
    lv_obj_set_style_text_font(pwd, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pwd, hex_to_lv_color(COLOR_TEXT_MUTED), 0);
    lv_obj_align(pwd, LV_ALIGN_CENTER, 0, 95);

    // IP address
    label_wifi_ip = lv_label_create(screen_wifi);
    lv_label_set_text(label_wifi_ip, "Then go to: 192.168.4.1");
    lv_obj_set_style_text_font(label_wifi_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_wifi_ip, hex_to_lv_color(COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(label_wifi_ip, LV_ALIGN_CENTER, 0, 130);
}

static lv_obj_t* create_service_row(lv_obj_t* parent, const TrainService& service) {
    // Container for the service row
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_add_style(row, &style_service_row, 0);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Top row: Time, Platform, Status
    lv_obj_t* top_row = lv_obj_create(row);
    lv_obj_remove_style_all(top_row);
    lv_obj_set_size(top_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Departure time
    lv_obj_t* time_label = lv_label_create(top_row);
    String timeStr = service.scheduledDeparture;
    if (service.delayMinutes > 0 && !service.isCancelled) {
        timeStr += " -> " + service.estimatedDeparture;
    }
    lv_label_set_text(time_label, timeStr.c_str());
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(time_label, hex_to_lv_color(COLOR_TEXT_PRIMARY), 0);

    // Platform
    lv_obj_t* plat_label = lv_label_create(top_row);
    String platStr = "Plat " + service.platform;
    lv_label_set_text(plat_label, platStr.c_str());
    lv_obj_set_style_text_font(plat_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(plat_label, hex_to_lv_color(COLOR_TEXT_SECONDARY), 0);

    // Status indicator
    lv_obj_t* status_label = lv_label_create(top_row);
    if (service.isCancelled) {
        lv_label_set_text(status_label, "CANCELLED");
    } else if (service.delayMinutes > 0) {
        char delayStr[16];
        snprintf(delayStr, sizeof(delayStr), "+%d min", service.delayMinutes);
        lv_label_set_text(status_label, delayStr);
    } else {
        lv_label_set_text(status_label, "On Time");
    }
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, hex_to_lv_color(train_status_to_color(service.status)), 0);

    // Bottom row: Operator and arrival time
    lv_obj_t* bot_row = lv_obj_create(row);
    lv_obj_remove_style_all(bot_row);
    lv_obj_set_size(bot_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(bot_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bot_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Operator
    lv_obj_t* op_label = lv_label_create(bot_row);
    lv_label_set_text(op_label, service.operatorName.c_str());
    lv_obj_set_style_text_font(op_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(op_label, hex_to_lv_color(COLOR_TEXT_MUTED), 0);

    // Arrival time
    if (service.scheduledArrival.length() > 0) {
        lv_obj_t* arr_label = lv_label_create(bot_row);
        String arrStr = "Arr: " + service.scheduledArrival;
        if (service.estimatedArrival.length() > 0 &&
            service.estimatedArrival != service.scheduledArrival &&
            !service.estimatedArrival.equalsIgnoreCase("On time")) {
            arrStr += " (" + service.estimatedArrival + ")";
        }
        lv_label_set_text(arr_label, arrStr.c_str());
        lv_obj_set_style_text_font(arr_label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(arr_label, hex_to_lv_color(COLOR_TEXT_MUTED), 0);
    }

    // Delay/cancel reason (if any)
    if (service.isCancelled && service.cancelReason.length() > 0) {
        lv_obj_t* reason = lv_label_create(row);
        lv_label_set_text(reason, service.cancelReason.c_str());
        lv_label_set_long_mode(reason, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(reason, lv_pct(100));
        lv_obj_set_style_text_font(reason, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(reason, hex_to_lv_color(COLOR_STATUS_CANCELLED), 0);
    } else if (service.delayMinutes > 5 && service.delayReason.length() > 0) {
        lv_obj_t* reason = lv_label_create(row);
        lv_label_set_text(reason, service.delayReason.c_str());
        lv_label_set_long_mode(reason, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(reason, lv_pct(100));
        lv_obj_set_style_text_font(reason, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(reason, hex_to_lv_color(COLOR_STATUS_DELAYED), 0);
    }

    return row;
}

static lv_obj_t* create_departure_panel(lv_obj_t* parent, const char* title,
                                         bool isOutbound, DepartureBoard* board) {
    // Panel container
    lv_obj_t* panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_add_style(panel, &style_panel, 0);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(48));
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Header
    lv_obj_t* header = lv_obj_create(panel);
    lv_obj_remove_style_all(header);
    if (isOutbound) {
        lv_obj_add_style(header, &style_header_outbound, 0);
    } else {
        lv_obj_add_style(header, &style_header_inbound, 0);
    }
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);

    lv_obj_t* header_label = lv_label_create(header);
    lv_label_set_text(header_label, title);
    lv_obj_set_style_text_font(header_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(header_label, hex_to_lv_color(COLOR_TEXT_PRIMARY), 0);
    lv_obj_center(header_label);

    // Service list (scrollable)
    lv_obj_t* list = lv_obj_create(panel);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 4, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    // Store reference to list
    if (isOutbound) {
        list_outbound = list;
    } else {
        list_inbound = list;
    }

    return panel;
}

static void create_main_screen() {
    screen_main = lv_obj_create(nullptr);
    lv_obj_add_style(screen_main, &style_bg, 0);
    lv_obj_set_flex_flow(screen_main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen_main, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(screen_main, 4, 0);
    lv_obj_set_style_pad_row(screen_main, 4, 0);

    // Status bar at top
    status_bar = lv_obj_create(screen_main);
    lv_obj_remove_style_all(status_bar);
    lv_obj_add_style(status_bar, &style_status_bar, 0);
    lv_obj_set_size(status_bar, lv_pct(100), 32);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // WiFi indicator
    label_wifi = lv_label_create(status_bar);
    lv_label_set_text(label_wifi, LV_SYMBOL_WIFI " Connected");
    lv_obj_set_style_text_font(label_wifi, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label_wifi, hex_to_lv_color(COLOR_TEXT_SECONDARY), 0);

    // Status text
    label_status = lv_label_create(status_bar);
    lv_label_set_text(label_status, "Ready");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label_status, hex_to_lv_color(COLOR_TEXT_MUTED), 0);

    // Last update time
    label_time = lv_label_create(status_bar);
    lv_label_set_text(label_time, "Updated: --:--");
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label_time, hex_to_lv_color(COLOR_TEXT_SECONDARY), 0);

    // Refresh button
    btn_refresh = lv_btn_create(status_bar);
    lv_obj_remove_style_all(btn_refresh);
    lv_obj_add_style(btn_refresh, &style_btn, 0);
    lv_obj_set_size(btn_refresh, 70, 26);

    lv_obj_t* btn_label = lv_label_create(btn_refresh);
    lv_label_set_text(btn_label, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_12, 0);
    lv_obj_center(btn_label);

    // Content area
    lv_obj_t* content = lv_obj_create(screen_main);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 4, 0);

    // Outbound panel (top)
    panel_outbound = create_departure_panel(content,
        STATION_LUTON_NAME " -> " STATION_FARRINGDON_NAME,
        true, nullptr);

    // Inbound panel (bottom)
    panel_inbound = create_departure_panel(content,
        STATION_FARRINGDON_NAME " -> " STATION_LUTON_NAME,
        false, nullptr);

    // Loading spinner overlay (hidden by default)
    spinner_loading = lv_spinner_create(screen_main, 1000, 60);
    lv_obj_set_size(spinner_loading, 60, 60);
    lv_obj_center(spinner_loading);
    lv_obj_add_flag(spinner_loading, LV_OBJ_FLAG_HIDDEN);
}

static void create_error_screen() {
    screen_error = lv_obj_create(nullptr);
    lv_obj_add_style(screen_error, &style_bg, 0);

    // Error icon
    lv_obj_t* icon = lv_label_create(screen_error);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, hex_to_lv_color(COLOR_STATUS_CANCELLED), 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -60);

    // Error title
    label_error_title = lv_label_create(screen_error);
    lv_label_set_text(label_error_title, "Error");
    lv_obj_set_style_text_font(label_error_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_error_title, hex_to_lv_color(COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(label_error_title, LV_ALIGN_CENTER, 0, 0);

    // Error message
    label_error_msg = lv_label_create(screen_error);
    lv_label_set_text(label_error_msg, "Something went wrong");
    lv_label_set_long_mode(label_error_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_error_msg, 400);
    lv_obj_set_style_text_font(label_error_msg, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_error_msg, hex_to_lv_color(COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_align(label_error_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_error_msg, LV_ALIGN_CENTER, 0, 50);
}

// =============================================================================
// Public Functions
// =============================================================================

void ui_init() {
    DEBUG_PRINTLN("Initializing UI...");

    // Initialize styles
    init_styles();

    // Create all screens
    create_splash_screen();
    create_wifi_screen();
    create_main_screen();
    create_error_screen();

    // Show splash screen initially
    lv_scr_load(screen_splash);

    DEBUG_PRINTLN("UI initialized");
}

void ui_show_screen(UIScreen screen) {
    switch (screen) {
        case SCREEN_SPLASH:
            lv_scr_load(screen_splash);
            break;
        case SCREEN_WIFI_SETUP:
            lv_scr_load(screen_wifi);
            break;
        case SCREEN_MAIN:
            lv_scr_load(screen_main);
            break;
        case SCREEN_ERROR:
            lv_scr_load(screen_error);
            break;
    }
}

void ui_update_train_data() {
    DepartureBoard* outbound = train_api_get_outbound();
    DepartureBoard* inbound = train_api_get_inbound();

    // Clear and rebuild outbound list
    if (list_outbound) {
        lv_obj_clean(list_outbound);

        if (outbound && outbound->hasData && outbound->services.size() > 0) {
            for (const auto& service : outbound->services) {
                create_service_row(list_outbound, service);
            }
        } else if (outbound && outbound->hasData) {
            lv_obj_t* no_trains = lv_label_create(list_outbound);
            lv_label_set_text(no_trains, "No trains in the next 2 hours");
            lv_obj_set_style_text_color(no_trains, hex_to_lv_color(COLOR_TEXT_MUTED), 0);
        } else if (outbound && outbound->errorMessage.length() > 0) {
            lv_obj_t* error = lv_label_create(list_outbound);
            lv_label_set_text(error, outbound->errorMessage.c_str());
            lv_obj_set_style_text_color(error, hex_to_lv_color(COLOR_STATUS_DELAYED), 0);
        } else {
            lv_obj_t* loading = lv_label_create(list_outbound);
            lv_label_set_text(loading, "Loading...");
            lv_obj_set_style_text_color(loading, hex_to_lv_color(COLOR_TEXT_MUTED), 0);
        }
    }

    // Clear and rebuild inbound list
    if (list_inbound) {
        lv_obj_clean(list_inbound);

        if (inbound && inbound->hasData && inbound->services.size() > 0) {
            for (const auto& service : inbound->services) {
                create_service_row(list_inbound, service);
            }
        } else if (inbound && inbound->hasData) {
            lv_obj_t* no_trains = lv_label_create(list_inbound);
            lv_label_set_text(no_trains, "No trains in the next 2 hours");
            lv_obj_set_style_text_color(no_trains, hex_to_lv_color(COLOR_TEXT_MUTED), 0);
        } else if (inbound && inbound->errorMessage.length() > 0) {
            lv_obj_t* error = lv_label_create(list_inbound);
            lv_label_set_text(error, inbound->errorMessage.c_str());
            lv_obj_set_style_text_color(error, hex_to_lv_color(COLOR_STATUS_DELAYED), 0);
        } else {
            lv_obj_t* loading = lv_label_create(list_inbound);
            lv_label_set_text(loading, "Loading...");
            lv_obj_set_style_text_color(loading, hex_to_lv_color(COLOR_TEXT_MUTED), 0);
        }
    }
}

void ui_show_loading(const char* message) {
    if (spinner_loading) {
        lv_obj_clear_flag(spinner_loading, LV_OBJ_FLAG_HIDDEN);
    }
    if (label_status && message) {
        lv_label_set_text(label_status, message);
    }
}

void ui_hide_loading() {
    if (spinner_loading) {
        lv_obj_add_flag(spinner_loading, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_show_error(const char* title, const char* message) {
    if (label_error_title) {
        lv_label_set_text(label_error_title, title);
    }
    if (label_error_msg) {
        lv_label_set_text(label_error_msg, message);
    }
    ui_show_screen(SCREEN_ERROR);
}

void ui_show_wifi_setup(const char* ssid, const char* ip) {
    if (label_wifi_ssid && ssid) {
        lv_label_set_text(label_wifi_ssid, ssid);
    }
    if (label_wifi_ip && ip) {
        String ipStr = "Then go to: " + String(ip);
        lv_label_set_text(label_wifi_ip, ipStr.c_str());
    }
    ui_show_screen(SCREEN_WIFI_SETUP);
}

void ui_update_status(const char* message) {
    if (label_status) {
        lv_label_set_text(label_status, message);
    }
}

void ui_update_wifi_status(bool connected, int rssi) {
    if (!label_wifi) return;

    if (connected) {
        String wifiStr = String(LV_SYMBOL_WIFI) + " ";
        if (rssi > -50) {
            wifiStr += "Excellent";
        } else if (rssi > -60) {
            wifiStr += "Good";
        } else if (rssi > -70) {
            wifiStr += "Fair";
        } else {
            wifiStr += "Weak";
        }
        lv_label_set_text(label_wifi, wifiStr.c_str());
        lv_obj_set_style_text_color(label_wifi, hex_to_lv_color(COLOR_STATUS_ON_TIME), 0);
    } else {
        lv_label_set_text(label_wifi, LV_SYMBOL_WARNING " No WiFi");
        lv_obj_set_style_text_color(label_wifi, hex_to_lv_color(COLOR_STATUS_DELAYED), 0);
    }
}

void ui_update_refresh_time(int secondsAgo) {
    if (!label_time) return;

    if (secondsAgo < 0) {
        lv_label_set_text(label_time, "Updated: Never");
    } else if (secondsAgo < 60) {
        lv_label_set_text(label_time, "Updated: Just now");
    } else {
        char buf[32];
        int minutes = secondsAgo / 60;
        snprintf(buf, sizeof(buf), "Updated: %dm ago", minutes);
        lv_label_set_text(label_time, buf);
    }
}

lv_obj_t* ui_get_refresh_button() {
    return btn_refresh;
}

void ui_process() {
    // Update refresh time indicator
    int secAgo = train_api_seconds_since_update();
    ui_update_refresh_time(secAgo);
}
