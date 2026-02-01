# UK Train Departure Display

A real-time train departure display for ESP32-4848S040, showing live train times between Luton and Farringdon using the National Rail Darwin API.

![ESP32-4848S040](https://img.shields.io/badge/Hardware-ESP32--4848S040-blue)
![Platform](https://img.shields.io/badge/Platform-ESP32--S3-green)
![Display](https://img.shields.io/badge/Display-480x480%20TFT-orange)

## Features

- **Live train data** - Real-time departures from National Rail Darwin API
- **Split-screen display** - Top half shows Luton → Farringdon, bottom shows return journey
- **Status indicators** - Color-coded: green (on time), amber (delayed), red (cancelled)
- **Auto-refresh** - Updates every 60 seconds automatically
- **Manual refresh** - Touch the refresh button for immediate updates
- **WiFi setup portal** - Easy configuration via captive portal
- **Delay reasons** - Shows why trains are delayed or cancelled when available
- **Dark theme** - Easy on the eyes, perfect for bedside/hallway placement

## Hardware

- **Board**: ESP32-4848S040 (ESP32-S3 based)
- **Display**: 4" 480x480 IPS LCD with ST7701 driver
- **Touch**: GT911 capacitive touchscreen
- **Memory**: 16MB Flash, 8MB PSRAM

## Prerequisites

### 1. National Rail API Token

You need a free API token from National Rail:

1. Go to [National Rail Developer Portal](https://www.nationalrail.co.uk/developers/)
2. Register for an account (may take 1-3 business days for approval)
3. Once approved, get your API token

### 2. Development Environment

Choose one of:

**Option A: GitHub Codespaces (Recommended - Cloud-based)**
1. Fork this repository
2. Open in GitHub Codespaces
3. Install PlatformIO extension when prompted

**Option B: Local Development**
1. Install [VS Code](https://code.visualstudio.com/)
2. Install [PlatformIO IDE Extension](https://platformio.org/install/ide?install=vscode)

## Building and Flashing

### Cloud Build (GitHub Codespaces)

1. Open terminal in Codespaces
2. Build the firmware:
   ```bash
   pio run
   ```
3. Download the firmware file: `.pio/build/esp32-4848S040/firmware.bin`
4. Flash using [ESP Web Tools](https://web.esptool.io/):
   - Connect ESP32 via USB
   - Open ESP Web Tools in Chrome/Edge
   - Click "Connect" and select your device
   - Click "Install" and select the downloaded `firmware.bin`

### Local Build

```bash
# Build
pio run

# Build and upload (with device connected)
pio run --target upload

# Monitor serial output
pio device monitor
```

## First-Time Setup

1. **Power on** the device via USB-C
2. **Connect to WiFi** - The display will show:
   - WiFi network: `TrainDisplay-Setup`
   - Password: `traintime`
3. **Configure** - Open `192.168.4.1` in your browser:
   - Select your home WiFi network
   - Enter your WiFi password
   - Enter your National Rail API token
   - Click Save
4. **Done!** - The device will connect and start showing train times

## Configuration

Edit `src/config.h` to customize:

### Stations
```cpp
// Change these to your stations
#define STATION_LUTON_CRS "LUT"
#define STATION_LUTON_NAME "Luton"
#define STATION_FARRINGDON_CRS "ZFD"
#define STATION_FARRINGDON_NAME "Farringdon"
```

Find station CRS codes at [National Rail](https://www.nationalrail.co.uk/stations_destinations/48702.aspx)

### Refresh Rate
```cpp
// Auto-refresh interval (default: 60 seconds)
#define AUTO_REFRESH_INTERVAL_MS 60000
```

### Time Window
```cpp
// Show trains for next N minutes (max 120)
#define DEPARTURE_TIME_WINDOW 120
```

### Colors
```cpp
// Customize the color scheme
#define COLOR_STATUS_ON_TIME  0x22c55e  // Green
#define COLOR_STATUS_DELAYED  0xf59e0b  // Amber
#define COLOR_STATUS_CANCELLED 0xdc2626 // Red
```

## Display Layout

```
┌─────────────────────────────────────────┐
│ WiFi: Good | Ready | Updated: 30s | [⟳] │  <- Status bar
├─────────────────────────────────────────┤
│         Luton → Farringdon              │  <- Outbound header
│ ─────────────────────────────────────── │
│ 07:15 → 07:18    Plat 3       +3 min   │
│ Thameslink            Arr: 07:52        │
│ ─────────────────────────────────────── │
│ 07:30              Plat 1      On Time  │
│ Thameslink            Arr: 08:05        │
├─────────────────────────────────────────┤
│        Farringdon → Luton               │  <- Inbound header
│ ─────────────────────────────────────── │
│ 17:45              Plat 2      On Time  │
│ Thameslink            Arr: 18:20        │
│ ─────────────────────────────────────── │
│ 18:00              Plat 4     CANCELLED │
│ Thameslink                              │
│ Short formation due to train fault      │
└─────────────────────────────────────────┘
```

## Troubleshooting

### WiFi won't connect
- Ensure you're within range of your router
- Try resetting: hold power button for 10 seconds
- The setup portal will reappear

### No train data / API errors
- Check your API token is correct
- Verify token is approved at [National Rail](https://www.nationalrail.co.uk/developers/)
- Check station CRS codes are valid

### Display issues
- Ensure using good quality USB-C cable
- Try different USB port (some have power issues)
- Check for loose connections

### Build errors
- Run `pio pkg update` to update libraries
- Delete `.pio` folder and rebuild
- Ensure PlatformIO is up to date

## API Information

This project uses the [Huxley2](https://github.com/jpsingleton/Huxley2) JSON proxy for the National Rail Darwin API:

- **Rate limits**: ~5 million requests per 28 days (very generous!)
- **Data source**: Official National Rail Live Departure Boards
- **Update frequency**: Real-time data

## Project Structure

```
├── platformio.ini          # PlatformIO configuration
├── include/
│   └── lv_conf.h          # LVGL configuration
├── src/
│   ├── main.cpp           # Application entry point
│   ├── config.h           # Configuration settings
│   ├── display.h/cpp      # Display driver (ST7701 + GT911)
│   ├── wifi_manager.h/cpp # WiFi with captive portal
│   ├── train_api.h/cpp    # National Rail API client
│   └── ui.h/cpp           # LVGL user interface
└── README.md
```

## License

MIT License - feel free to modify and use for your own projects!

## Acknowledgments

- [National Rail Enquiries](https://www.nationalrail.co.uk/) for the Darwin API
- [Huxley2](https://github.com/jpsingleton/Huxley2) for the JSON proxy
- [LVGL](https://lvgl.io/) for the graphics library
- [Arduino_GFX](https://github.com/moononournation/Arduino_GFX) for display drivers
