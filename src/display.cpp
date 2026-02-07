/**
 * Display Driver Implementation for ESP32-4848S040
 * 4" 480x480 ST7701 RGB LCD with GT911 Touch
 *
 * CRITICAL: This board requires BOTH a 9-bit SPI bus (for ST7701 register
 * configuration) AND a 16-bit RGB parallel bus (for pixel data).
 * The ST7701 init commands are sent via SPI, then the RGB interface takes over.
 *
 * Pin mapping verified against manufacturer schematics and community sources:
 * - https://github.com/moononournation/Arduino_GFX/issues/465
 * - https://devices.esphome.io/devices/guition-esp32-s3-4848s040/
 */

#include "display.h"
#include "config.h"
#include <Arduino_GFX_Library.h>
#include <Wire.h>

// =============================================================================
// PIN DEFINITIONS for ESP32-4848S040
// Verified against manufacturer schematic and community-tested configurations
// =============================================================================

// ST7701 SPI bus pins (9-bit SPI for register configuration)
#define LCD_SPI_CS   39
#define LCD_SPI_SCK  48
#define LCD_SPI_MOSI 47

// RGB parallel interface control pins
#define LCD_DE    18
#define LCD_VSYNC 17
#define LCD_HSYNC 16
#define LCD_PCLK  21

// RGB data pins - 16-bit (R5 + G6 + B5)
// Red channel (5 bits)
#define LCD_R0 11
#define LCD_R1 12
#define LCD_R2 13
#define LCD_R3 14
#define LCD_R4 0

// Green channel (6 bits)
#define LCD_G0 8
#define LCD_G1 20
#define LCD_G2 3
#define LCD_G3 46
#define LCD_G4 9
#define LCD_G5 10

// Blue channel (5 bits)
#define LCD_B0 4
#define LCD_B1 5
#define LCD_B2 6
#define LCD_B3 7
#define LCD_B4 15

// Backlight control (drives boost converter enable via GPIO)
#define LCD_BL 38

// Touch I2C pins (GT911 capacitive touch controller)
#define TOUCH_SDA 19
#define TOUCH_SCL 45

// GT911 I2C address
#define GT911_ADDR 0x5D

// =============================================================================
// Display and LVGL objects
// =============================================================================

// Arduino GFX objects
static Arduino_DataBus* spi_bus = nullptr;
static Arduino_ESP32RGBPanel* rgbpanel = nullptr;
static Arduino_RGB_Display* gfx = nullptr;

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

    // Initialize backlight pin - start OFF
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);

    // Set up PWM for backlight dimming
    ledcSetup(BACKLIGHT_CHANNEL, BACKLIGHT_FREQ, BACKLIGHT_RESOLUTION);
    ledcAttachPin(LCD_BL, BACKLIGHT_CHANNEL);
    ledcWrite(BACKLIGHT_CHANNEL, 0);

    // Initialize I2C for touch (SDA=19, SCL=45 per schematic)
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(400000);

    // =========================================================================
    // Create SPI bus for ST7701 register initialization.
    // The ST7701 requires a 9-bit SPI interface to receive its init commands.
    // Without this bus, the display controller never leaves sleep mode and
    // the screen stays completely blank!
    // =========================================================================
    spi_bus = new Arduino_SWSPI(
        GFX_NOT_DEFINED, // DC - not used in 9-bit SPI mode
        LCD_SPI_CS,      // CS  = GPIO 39
        LCD_SPI_SCK,     // SCK = GPIO 48
        LCD_SPI_MOSI,    // MOSI = GPIO 47
        GFX_NOT_DEFINED  // MISO - not used
    );

    // =========================================================================
    // Create RGB panel with correct pin mapping and timing parameters.
    // These values are verified from community-tested configurations.
    // =========================================================================
    rgbpanel = new Arduino_ESP32RGBPanel(
        LCD_DE, LCD_VSYNC, LCD_HSYNC, LCD_PCLK,
        LCD_R0, LCD_R1, LCD_R2, LCD_R3, LCD_R4,
        LCD_G0, LCD_G1, LCD_G2, LCD_G3, LCD_G4, LCD_G5,
        LCD_B0, LCD_B1, LCD_B2, LCD_B3, LCD_B4,
        1,    // hsync_polarity (active high)
        10,   // hsync_front_porch
        8,    // hsync_pulse_width
        50,   // hsync_back_porch
        1,    // vsync_polarity (active high)
        10,   // vsync_front_porch
        8,    // vsync_pulse_width
        20    // vsync_back_porch
    );

    // =========================================================================
    // Create display driver. The SPI bus sends ST7701 register configuration,
    // then the RGB panel takes over for pixel data.
    // =========================================================================
    gfx = new Arduino_RGB_Display(
        SCREEN_WIDTH, SCREEN_HEIGHT,
        rgbpanel,
        0,                // rotation
        true,             // auto_flush
        spi_bus,          // SPI bus for ST7701 init (was nullptr - CRITICAL FIX)
        GFX_NOT_DEFINED,  // RST (handled by hardware power-on reset circuit)
        st7701_type8_init_operations,
        sizeof(st7701_type8_init_operations)
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
        DEBUG_PRINTLN("Failed to allocate display buffers in PSRAM!");
        // Try without PSRAM
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
        buf2 = nullptr;
        if (!buf1) {
            DEBUG_PRINTLN("Failed to allocate any display buffer!");
            return false;
        }
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
