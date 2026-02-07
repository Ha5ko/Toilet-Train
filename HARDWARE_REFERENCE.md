# ESP32-4848S040 Hardware Reference

> Comprehensive hardware specification for the 4.0" ESP32 display module used in
> this project. Compiled from manufacturer datasheets, schematics, and operating
> instructions from Shenzhen Jingcai Intelligent Co., Ltd.

---

## Board Overview

| Parameter | Value |
|---|---|
| Manufacturer | Shenzhen Jingcai Intelligent Co., Ltd (深圳市晶彩智能有限公司) |
| Model | ESP32-4848S040 |
| SKU (1 relay) | ESP32-4848S040C_I_Y_1 |
| SKU (3 relay) | ESP32-4848S040C_I_Y_3 |
| Dimensions | 86.5 x 86.5 x 13.6 mm |
| Weight | ~200g |
| Operating temp | -20°C to 70°C |
| Storage temp | -30°C to 80°C |
| Operating voltage | 5V (via USB-C) |
| Power consumption | ~260mA |

---

## MCU: ESP32-S3-WROOM-1

| Parameter | Value |
|---|---|
| SoC | ESP32-S3 |
| Architecture | Xtensa dual-core 32-bit LX7 |
| Clock speed | 240 MHz |
| SRAM | 512 KB (16 KB in RTC) |
| ROM | 384 KB |
| PSRAM | 8 MB (OPI) |
| Flash | 16 MB |
| Wi-Fi | 802.11 b/g/n 2.4 GHz |
| Bluetooth | BLE 5 + Mesh |
| GPIOs | 45 |
| SPI | 4x |
| UART | 3x |
| I2C | 2x |
| I2S | 2x |
| ADC | 2x 12-bit |
| Touch channels | 14 |
| Other | RMT, LED PWM, USB-OTG, TWAI, DVP, LCD interface |

---

## Display

| Parameter | Value |
|---|---|
| Size | 4.0 inches |
| Type | IPS TFT |
| Resolution | 480 x 480 pixels |
| Driver IC | ST7701 |
| Interface | 16-bit RGB (directly driven by ESP32-S3 LCD peripheral) |
| Color depth | 16-bit RGB 65K colors |
| Effective display area | 71.8 x 70.2 mm |
| Viewing angle | >60° |
| Backlight control | PWM via LED driver circuit on IO38 |

### ST7701 Initialization

The ST7701 requires a specific command sequence sent before the RGB interface
becomes active. The init commands are defined in `src/display.cpp` as
`st7701_init_cmd[]`. Key steps:

1. Enter Command2 BK0 (0xFF, 0x77, 0x01, 0x00, 0x00, 0x10) - gamma settings
2. Enter Command2 BK1 (0xFF, 0x77, 0x01, 0x00, 0x00, 0x11) - power/timing
3. Return to user command set (0xFF, 0x77, 0x01, 0x00, 0x00, 0x00)
4. Sleep out (0x11)

The RGB timing parameters used:
- HSYNC/VSYNC polarity: 0 (active low)
- Front porch: 8
- Pulse width: 4
- Back porch: 8
- PCLK active negative: 1
- Pixel clock: 16 MHz

---

## Touch Controller: GT911

| Parameter | Value |
|---|---|
| IC | Goodix GT911 |
| Type | Capacitive |
| Interface | I2C |
| I2C address | 0x5D |
| SDA pin | IO19 |
| SCL pin | IO20 |
| INT pin | IO18 |
| I2C speed | 400 kHz |

### GT911 Register Map (key registers)

| Register | Description |
|---|---|
| 0x814E | Buffer status (bit 7 = data ready, bits 3:0 = touch count) |
| 0x8150 | Touch point 1 data (X low, X high, Y low, Y high) |

### Touch Read Sequence

1. Read register 0x814E for status
2. If bit 7 is set and touch count > 0, read 4 bytes from 0x8150
3. Clear status by writing 0x00 to register 0x814E

---

## Complete Pin Mapping

### LCD RGB Data Bus (active accent - accent accent)

| Signal | GPIO | Function |
|---|---|---|
| R0 | IO45 | Red bit 0 (LSB) |
| R1 | IO48 | Red bit 1 |
| R2 | IO47 | Red bit 2 |
| R3 | IO21 | Red bit 3 |
| R4 | IO14 | Red bit 4 (MSB) |
| G0 | IO5 | Green bit 0 (LSB) |
| G1 | IO6 | Green bit 1 |
| G2 | IO7 | Green bit 2 |
| G3 | IO15 | Green bit 3 |
| G4 | IO16 | Green bit 4 |
| G5 | IO4 | Green bit 5 (MSB) |
| B0 | IO8 | Blue bit 0 (LSB) |
| B1 | IO3 | Blue bit 1 |
| B2 | IO46 | Blue bit 2 |
| B3 | IO9 | Blue bit 3 |
| B4 | IO1 | Blue bit 4 (MSB) |

### LCD Control Signals

| Signal | GPIO | Function |
|---|---|---|
| DE | IO40 | Data Enable |
| VSYNC | IO41 | Vertical Sync |
| HSYNC | IO42 | Horizontal Sync |
| PCLK | IO39 | Pixel Clock |
| BL_CTR | IO38 | Backlight PWM control |

### Touch (I2C)

| Signal | GPIO | Pull-up |
|---|---|---|
| TP_SDA | IO19 | 4.7K to 3.3V (R4) |
| TP_SCL | IO20 | 4.7K to 3.3V (R3) |
| TP_INT | IO18 | - |
| LCD_RST | IO45 | Via FPC connector pin 4 |

### TF / SD Card (SPI)

| Signal | GPIO | Notes |
|---|---|---|
| TF_CS | IO42 | Directly shared with HSYNC |
| MCU_MOSI | IO47 | Directly shared with R2 |
| TF_CLK | IO48 | Directly shared with R1 |
| MCU_MISO | IO41 | Directly shared with VSYNC |

> **Important**: The SD card shares pins with the LCD control and data lines.
> Using the SD card while the display is active requires careful multiplexing
> or is typically done only during init/shutdown.

### Relay / Audio (SHARED - mutually exclusive)

These three GPIOs serve **either** the relays **or** the I2S audio amplifier,
depending on which 0-ohm resistors are populated:

| GPIO | Relay function (default) | Audio function (alternate) | Relay resistor | Audio resistor |
|---|---|---|---|---|
| IO40 | relay1 (via R25) | I2S DIN (via R21) | R25 (default populated) | R21 |
| IO2 | relay2 (via R26) | I2S LRCLK (via R22) | R26 (default populated) | R22 |
| IO1 | relay3 (via R27) | I2S BCLK (via R23) | R27 (default populated) | R23 |

To enable audio: move 0-ohm resistors R25/R26/R27 to R21/R22/R23.

The audio amplifier is an **NS4168** (I2S input, Class-D output):
- CTRL pin directly connected
- BCLK, LRCLK, DIN from ESP32 via resistor selection
- VDD from VOUT-BAT
- Speaker output via JST 1.25mm 2P connector (P7)

> **Note**: IO40 is also shared with LCD DE (Data Enable). The relay/audio
> and LCD DE use the same physical pin. IO1 is shared with LCD B4.
> This means the display uses these pins simultaneously with their LCD
> function, and the relay/audio routing is on a separate trace selected
> by the resistor placement.

### USB Interface

| Component | Function |
|---|---|
| USB1 (Type-C) | Power input + data |
| CH340C | USB-to-TTL serial converter |
| U0TXD | ESP32 UART0 TX → CH340 RXD |
| U0RXD | ESP32 UART0 RX → CH340 TXD |

The CH340C provides the serial programming interface. The ESP32-S3 also
has native USB but the board routes through CH340C for compatibility.

Auto-reset circuit: RTS → T1 (NPN) → IO0 (BOOT), DTR → T2 (NPN) → RST.

### Expansion Header (P2)

The board has a header connector P2 with these pins:

| Pin | Signal |
|---|---|
| 1 | U0RXD |
| 2 | U0TXD |
| 3 | IO9 |
| 4 | RST |
| 5 | GND |
| 6 | 3.3V |
| 7 | GND |

### Relay Header (H1)

| Pin | Signal |
|---|---|
| 1 | GND |
| 2 | GND |
| 3 | relay1 |
| 4 | U0RXD |
| 5 | relay2 |
| 6 | U0TXD |
| 7 | relay3 |
| 8 | 5V |

### Other Pins

| Signal | GPIO | Notes |
|---|---|---|
| BOOT button | IO0 | SW_3x4, active low, directly on ESP32-S3 strapping pin |
| RST button | - | Connected to EN/RST pin |
| IO0 | IO0 | Also directly accessible, active low for boot mode |

---

## Power System

### USB-C Power Path
- 5V from USB-C VBUS → main power rail
- Protected with TVS diode (D3, SMBJ6.0CA)
- MOSFET power switch (Q1, AO3401) controlled by VEN

### LED Backlight Driver
- Dedicated boost converter (U5) for backlight LEDs
- VIN from VOUT-BAT rail
- EN controlled by IO38 (BL_CTR) via R13
- Schottky diode D2 on output
- Output filtered by C13/C14
- Drives LEDA/LEDK (anode/cathode of LED strip)

### Lithium Battery Charging
- Charging IC (U1) with LED indicators (LED1, LED2, LED3)
- Battery connector: JST 1.25mm 2P (P6)
- Charge path: 5V → U1 → BAT+ / BAT-
- VOUT-BAT rail feeds both battery and main board power
- Inductor L2 for regulation
- KEY/SW button (SW1) for power control

### Voltage Regulation
- 3.3V regulator with input/output filtering
- Multiple decoupling capacitors (C11, C12, C16, C24)
- Ferrite beads for noise isolation

---

## Partition Tables

### Manufacturer's huge_app.csv (for maximum app size)

```
# Name,   Type, SubType, Offset,  Size,    Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  factory, 0x10000, 0x7E0000,
coredump, data, coredump,0x7F0000,0x10000,
```

- **app0**: ~7.875 MB (single factory partition, no OTA)
- **coredump**: 64 KB for crash dumps
- **Total usable flash**: ~8 MB for application

### Project's current partition (default_16MB.csv)

The PlatformIO config currently uses `default_16MB.csv` which includes OTA
partitions. Switch to `huge_app.csv` if firmware exceeds the default app
partition size.

To use the manufacturer's partition table, set in `platformio.ini`:
```ini
board_build.partitions = huge_app.csv
```
And place `huge_app.csv` in the project root.

---

## Flash / Burn Instructions

### Using ESP32-S3 Flash Download Tool (v3.9.3)

1. **Open tool** - Select: ChipType = `ESP32-S3`, WorkMode = `Develop`, LoadMode = `UART`
2. **Configure**:
   - Browse to `.bin` file, set address to `0x0`
   - SPI Speed: `80 MHz`
   - SPI Mode: `QIO`
   - Check `DoNotChgBin`
3. **Connect**: Select COM port, Baud = `921600`
4. **Flash**: Click `START`
5. **Verify**: After burning, press RST button or power cycle to boot

### Using PlatformIO (development)

```bash
# Build firmware
pio run

# Build and flash (USB connected)
pio run --target upload

# Monitor serial output
pio device monitor
```

Upload speed configured at 921600 baud in `platformio.ini`.

---

## Development Environment Requirements

- **ESP32 Arduino Core**: >= 2.0.6 (project uses espressif32@^6.9.0)
- **PlatformIO**: Recommended IDE
- **Arduino IDE**: Also supported (replace partition table and boards.txt per manufacturer instructions)
- **ESP-IDF**: Supported but not used in this project
- **MicroPython**: Supported by the hardware

### Arduino IDE Setup (from manufacturer instructions)

1. Replace partition table `huge_app.csv` at:
   `{Arduino15}/packages/esp32/hardware/esp32/{version}/tools/partitions/`
2. Replace `boards.txt` at:
   `{Arduino15}/packages/esp32/hardware/esp32/{version}/`
3. Select board "ESP32S3 Dev Module" with:
   - PSRAM: OPI PSRAM
   - Flash Size: 16MB
   - Partition Scheme: Huge APP
   - Flash Mode: QIO 80MHz

---

## Pre-built Firmware Binaries (in `Burn files/`)

| File | Size | Description |
|---|---|---|
| 4.0_LvglWidgets.bin | 638 KB | LVGL widget demo |
| 86Switch_onoff_v1.3.bin | 5.2 MB | Relay switch control application |
| JC4848W540C_I_W-V2.1-NEWUI.bin | 4.6 MB | Manufacturer's default OS (smart home UI) |
| switch86_lvgl_music.bin | 3.0 MB | Music player demo with LVGL |

These can be flashed using the ESP32-S3 Flash Download Tool at address 0x0.

---

## Key Design Notes

1. **Pin sharing is extensive** - The SD card, LCD, relay, and audio all share
   GPIO pins. This is a 4-layer PCB optimized for cost, not flexibility.

2. **IO40 triple duty** - This pin serves as LCD DE (always), and either
   relay1 or I2S DIN depending on resistor config.

3. **IO1 double duty** - Serves as LCD B4 (blue bit 4) and either relay3
   or I2S BCLK.

4. **No dedicated SPI bus for display** - The ST7701 uses the ESP32-S3's
   built-in RGB LCD peripheral, which drives 16 data lines + 4 control
   lines in parallel. This is not SPI - it's a parallel RGB interface.

5. **PSRAM is OPI (Octal)** - Must use `dio_opi` memory type in build config,
   not `qio_qspi`.

6. **Touch RST line** - The FPC connector shows LCD_RST on pin 4 going to
   IO45, which is also R0 of the LCD. The touch and LCD share reset circuitry.

7. **Backlight is not direct GPIO** - IO38 controls a boost converter (U5)
   via its EN pin through R13. PWM on IO38 controls brightness by
   enabling/disabling the boost converter at the PWM frequency.

8. **CH340C for programming** - The board uses a USB-to-serial chip rather
   than the ESP32-S3's native USB. This means the serial port appears as
   a standard COM port on the host.

---

## Schematic Blocks Reference

The full schematics (in `5-IO pin distribution/1.png` and `2.png`) contain
these circuit blocks:

### Sheet 1 (1.png)
- **POWER** - USB-C connector, TVS protection, power MOSFET switch
- **USB to TTL** - CH340C circuit with RS232-level signals, auto-reset
- **LED Driver** - Boost converter for backlight, driven by IO38
- **SD_Card** - TF card slot with SPI connections (IO42/47/48/41)

### Sheet 2 (2.png)
- **ESP32 + LCM** - Main ESP32-S3-WROOM-1 module with all GPIO breakout,
  connected to LCD1 (4.0 TFT) via 16-bit RGB bus
- **Capacitive Touch** - GT911 on FPC connector (6-pin), I2C to IO19/IO20
- **Lithium Battery Charging** - Charging IC with status LEDs, JST connector
- **Relay / Audio** - Shared IO1/IO2/IO40 with resistor-selectable routing
- **Expansion headers** - P2 (7-pin) and H1 (8-pin relay header)
