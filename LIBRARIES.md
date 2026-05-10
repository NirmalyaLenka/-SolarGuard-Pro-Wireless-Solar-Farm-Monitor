# Required Libraries

Install all of these via **Arduino IDE → Tools → Manage Libraries**
or in `platformio.ini` (see below).

## Arduino Library Manager

| Library Name | Author | Version | Notes |
|-------------|--------|---------|-------|
| `Adafruit SSD1306` | Adafruit | ≥2.5.0 | OLED driver |
| `Adafruit GFX Library` | Adafruit | ≥1.11.0 | Required by SSD1306 |
| `DallasTemperature` | Miles Burton | ≥3.9.0 | DS18B20 |
| `OneWire` | Paul Stoffregen | ≥2.3.7 | DS18B20 bus |
| `DHT sensor library` | Adafruit | ≥1.4.4 | DHT22 |
| `Adafruit Unified Sensor` | Adafruit | ≥1.1.9 | Required by DHT |
| `BH1750` | Christopher Laws | ≥1.3.0 | Light sensor |
| `INA226` | Rob Tillaart | ≥0.5.0 | Current/voltage |

## PlatformIO (platformio.ini)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.9
    milesburton/DallasTemperature@^3.9.0
    paulstoffregen/OneWire@^2.3.7
    adafruit/DHT sensor library@^1.4.4
    adafruit/Adafruit Unified Sensor@^1.1.14
    claws/BH1750@^1.3.0
    robtillaart/INA226@^0.5.0
monitor_speed = 115200
```

## ESP32 Board Support

Make sure you have **ESP32 by Espressif Systems** installed:
- Arduino IDE: **File → Preferences → Additional Board Manager URLs**
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Then: **Tools → Board → Boards Manager** → search "esp32" → Install
