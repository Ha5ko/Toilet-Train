/**
 * Display Driver Implementation for ESP32-4848S040
 * 4" 480x480 ST7701 RGB LCD with GT911 Touch
 */

#include "display.h"
#include "config.h"
#include <Arduino_GFX_Library.h>
#include <Wire.h>

// =============================================================================
// PIN DEFINITIONS for ESP32-4848S040
// =============================================================================

// ST7701 LCD pins (RGB interface)
#define LCD_DE    40
#define LCD_VSYNC 41
#define LCD_HSYNC 42
#define LCD_PCLK  39

// RGB data pins (directly from ESP32-S3 LCD peripheral)
#define LCD_R0 45
#define LCD_R1 48
#define LCD_R2 47
#define LCD_R3 21
#define LCD_R4 14

#define LCD_G0 5
#define LCD_G1 6
#define LCD_G2 7
#define LCD_G3 15
#define LCD_G4 16
#define LCD_G5 4

#define LCD_B0 8
#define LCD_B1 3
#define LCD_B2 46
#define LCD_B3 9
#define LCD_B4 1

// Backlight
#define LCD_BL 38

// Touch I2C pins (GT911)
#define TOUCH_SDA 19
#define TOUCH_SCL 20
#define TOUCH_INT 18
#define TOUCH_RST 38  // Shared with backlight on some variants

// GT911 I2C address
#define GT911_ADDR 0x5D

// =============================================================================
// Display and LVGL buffers
// =============================================================================

// Arduino GFX setup for ST7701
Arduino_ESP32RGBPanel* rgbpanel = nullptr;
Arduino_RGB_Display* gfx = nullptr;

// LVGL display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1 = nullptr;
static lv_color_t* buf2 = nullptr;

// LVGL display driver
static lv_disp_drv_t disp_drv;

// LVGL touch driver
static lv_indev_drv_t indev_drv;

// Touch state
static bool touch_pressed = false;
static uint16_t touch_x = 0;
static uint16_t touch_y = 0;

// =============================================================================
// ST7701 Initialization Sequence
// =============================================================================

// ST7701 init commands for ESP32-4848S040
static const uint8_t st7701_init_cmd[] = {
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
    0xC0, 2, 0x3B, 0x00,
    0xC1, 2, 0x0D, 0x02,
    0xC2, 2, 0x31, 0x05,
    0xCD, 1, 0x00,
    0xB0, 16, 0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18,
    0xB1, 16, 0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18,
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x11,
    0xB0, 1, 0x60,
    0xB1, 1, 0x32,
    0xB2, 1, 0x07,
    0xB3, 1, 0x80,
    0xB5, 1, 0x49,
    0xB7, 1, 0x85,
    0xB8, 1, 0x21,
    0xC1, 1, 0x78,
    0xC2, 1, 0x78,
    0xE0, 3, 0x00, 0x1B, 0x02,
    0xE1, 11, 0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44,
    0xE2, 12, 0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00,
    0xE3, 4, 0x00, 0x00, 0x11, 0x11,
    0xE4, 2, 0x44, 0x44,
    0xE5, 16, 0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0,
    0xE6, 4, 0x00, 0x00, 0x11, 0x11,
    0xE7, 2, 0x44, 0x44,
    0xE8, 16, 0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0,
    0xEB, 7, 0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40,
    0xEC, 2, 0x3C, 0x00,
    0xED, 16, 0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA,
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    0x11, 0,  // Sleep out
    0xFF, 0xFF  // End marker
};

// =============================================================================
// LVGL Flush Callback
// =============================================================================

static void lvgl_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)&color_p->full, w, h);

    lv_disp_flush_ready(drv);
}

// =============================================================================
// GT911 Touch Functions
// =============================================================================

static bool gt911_read_touch() {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(0x81);
    Wire.write(0x4E);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    Wire.requestFrom(GT911_ADDR, 1);
    if (Wire.available()) {
        uint8_t status = Wire.read();
        if ((status & 0x80) && ((status & 0x0F) > 0)) {
            // Read touch coordinates
            Wire.beginTransmission(GT911_ADDR);
            Wire.write(0x81);
            Wire.write(0x50);
            Wire.endTransmission();

            Wire.requestFrom(GT911_ADDR, 4);
            if (Wire.available() >= 4) {
                uint8_t data[4];
                for (int i = 0; i < 4; i++) {
                    data[i] = Wire.read();
                }
                touch_x = data[0] | (data[1] << 8);
                touch_y = data[2] | (data[3] << 8);
                touch_pressed = true;
            }

            // Clear status
            Wire.beginTransmission(GT911_ADDR);
            Wire.write(0x81);
            Wire.write(0x4E);
            Wire.write(0x00);
            Wire.endTransmission();

            return true;
        } else {
            // Clear status
            Wire.beginTransmission(GT911_ADDR);
            Wire.write(0x81);
            Wire.write(0x4E);
            Wire.write(0x00);
            Wire.endTransmission();
        }
    }

    touch_pressed = false;
    return false;
}

static void lvgl_touch_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    if (gt911_read_touch()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_x;
        data->point.y = touch_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// =============================================================================
// Public Functions
// =============================================================================

bool display_init() {
    DEBUG_PRINTLN("Initializing display...");

    // Initialize backlight
    pinMode(LCD_BL, OUTPUT);
    ledcSetup(BACKLIGHT_CHANNEL, BACKLIGHT_FREQ, BACKLIGHT_RESOLUTION);
    ledcAttachPin(LCD_BL, BACKLIGHT_CHANNEL);
    ledcWrite(BACKLIGHT_CHANNEL, 0);  // Start with backlight off

    // Initialize I2C for touch
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(400000);

    // Initialize RGB panel
    rgbpanel = new Arduino_ESP32RGBPanel(
        LCD_DE, LCD_VSYNC, LCD_HSYNC, LCD_PCLK,
        LCD_R0, LCD_R1, LCD_R2, LCD_R3, LCD_R4,
        LCD_G0, LCD_G1, LCD_G2, LCD_G3, LCD_G4, LCD_G5,
        LCD_B0, LCD_B1, LCD_B2, LCD_B3, LCD_B4,
        0,    // hsync_polarity
        8,    // hsync_front_porch
        4,    // hsync_pulse_width
        8,    // hsync_back_porch
        0,    // vsync_polarity
        8,    // vsync_front_porch
        4,    // vsync_pulse_width
        8,    // vsync_back_porch
        1,    // pclk_active_neg
        16000000  // prefer_speed
    );

    // Initialize GFX display
    gfx = new Arduino_RGB_Display(
        SCREEN_WIDTH, SCREEN_HEIGHT,
        rgbpanel,
        0,     // rotation
        true,  // auto_flush
        nullptr,  // bus (not used for RGB)
        -1,    // rst
        st7701_init_cmd,
        sizeof(st7701_init_cmd)
    );

    if (!gfx->begin()) {
        DEBUG_PRINTLN("Display initialization failed!");
        return false;
    }

    gfx->fillScreen(BLACK);
    DEBUG_PRINTLN("Display initialized");

    // Initialize LVGL
    lv_init();
    DEBUG_PRINTLN("LVGL initialized");

    // Allocate display buffers in PSRAM
    size_t buf_size = SCREEN_WIDTH * 40;  // 40 lines buffer
    buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!buf1 || !buf2) {
        DEBUG_PRINTLN("Failed to allocate display buffers!");
        // Try without PSRAM
        buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
        buf2 = nullptr;
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size);

    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize touch driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    // Turn on backlight
    display_set_brightness(BACKLIGHT_DEFAULT_BRIGHTNESS);

    DEBUG_PRINTLN("Display setup complete");
    return true;
}

void display_update() {
    lv_timer_handler();
}

void display_set_brightness(uint8_t brightness) {
    ledcWrite(BACKLIGHT_CHANNEL, brightness);
}

bool display_get_touch(uint16_t* x, uint16_t* y) {
    if (touch_pressed && x && y) {
        *x = touch_x;
        *y = touch_y;
        return true;
    }
    return false;
}
