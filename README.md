# ☀️ SolarGuard Pro — Wireless Solar Farm Monitor

> A complete open-source wireless solar panel monitoring system with OLED display, multi-sensor support, configurable alerts, and 3-button navigation.

---

## 📸 System Preview

```
┌──────────────────────┐         ESP-NOW (no router)       ┌──────────────────────┐
│   SENSOR NODE (TX)   │  ─────────────────────────────►  │  DISPLAY UNIT (RX)   │
│  (at solar panel)    │                                   │  (control room/wall) │
│                      │                                   │                      │
│  ┌──────────────┐   │                                   │  ┌────────────────┐  │
│  │   INA226     │   │                                   │  │  1.3" OLED     │  │
│  │  V + I meter │   │                                   │  │  128×64 px     │  │
│  └──────────────┘   │                                   │  └────────────────┘  │
│  ┌──────────────┐   │                                   │                      │
│  │   DS18B20    │   │                                   │  [PREV] [NEXT] [SET] │
│  │  Panel Temp  │   │                                   │                      │
│  └──────────────┘   │                                   └──────────────────────┘
│  ┌──────────────┐   │
│  │   DHT22      │   │
│  │  Ambient T/H │   │
│  └──────────────┘   │
│  ┌──────────────┐   │
│  │   BH1750     │   │
│  │   Lux meter  │   │
│  └──────────────┘   │
│  ┌──────────────┐   │
│  │  GP2Y1010    │   │
│  │  Dust sensor │   │
│  └──────────────┘   │
└──────────────────────┘
```

---

## ✨ Features

| Feature | Details |
|---------|---------|
| 🔋 Voltage monitoring | Real-time panel voltage with low/high alerts |
| ⚡ Current & Power | Voltage and current measured by INA226 with an external shunt resistor; calibrate for the installed shunt |
| 🌡️ Temperature | Panel surface (DS18B20) and ambient temperature (DHT22); humidity is read but not included in telemetry |
| 💡 Light level | BH1750 light measurement in lux; this is not calibrated solar irradiance |
| 🌫️ Soiling estimate | Experimental 0–100 index derived from GP2Y1010 analog output; not a calibrated dust measurement |
| 🧹 Wash alert | Auto alert when soiling exceeds threshold |
| ⏱️ Uptime tracking | Panel node uptime displayed in d/h/m/s |
| 📡 Wireless | ESP-NOW (peer-to-peer, no Wi-Fi router needed) |
| 📟 OLED display | 11 navigable screens on 128×64 OLED |
| 🔘 3-button UI | PREV / NEXT / SET buttons with set mode |
| 💾 Persistent config | Alert thresholds saved to NVS flash |
| 🔌 Signal strength | RSSI displayed for link quality |

---

## 🗂️ Repository Structure

The checked-in files are at the repository root; the directory tree shown below reflects the current checkout, not a proposed layout.

- `solar_monitor.ino` — receiver/display firmware
- `transmitter.ino` — transmitter/sensor-node firmware
- `platformio.ini` — two PlatformIO environments; its source filters currently expect files under `src/`, which does not match the checked-in root-level sketches
- `receiver_schematic.md` and `transmitter_schematic.md` — wiring references
- `LIBRARIES.md` — dependency list
- `TROUBLESHOOTING.md` — troubleshooting and calibration notes
- `index.html` — browser companion page
- `README.md` — this guide

## 🛒 Hardware BOM (Bill of Materials)

### Per Sensor Node (at each panel)
| Component | Qty | Purpose |
|-----------|-----|---------|
| ESP32 DevKit V1 | 1 | MCU + ESP-NOW radio |
| INA226 module | 1 | Voltage + current measurement |
| DS18B20 waterproof | 1 | Panel surface temperature |
| DHT22 module | 1 | Ambient temperature + humidity |
| BH1750 module | 1 | Light irradiance (lux) |
| GP2Y1010AU0F | 1 | Dust/soiling optical sensor |
| 4.7kΩ resistor | 1 | DS18B20 pull-up |
| 150Ω resistor | 1 | GP2Y1010 LED current limit |
| 220µF capacitor | 1 | GP2Y1010 filtering |
| TP4056 charger module | 1 | LiPo charging from solar |
| 18650 LiPo cell | 1 | Energy buffer / backup |
| Mini solar panel | 1 | Self-powered node |
| Weatherproof enclosure | 1 | IP65 rated box |

### Display Unit (receiver — one per farm)
| Component | Qty | Purpose |
|-----------|-----|---------|
| ESP32 DevKit V1 | 1 | MCU + ESP-NOW receiver |
| SSD1306/SH1106 OLED 1.3" | 1 | 128×64 I2C display |
| Momentary push button | 3 | PREV / NEXT / SET navigation |
| 10kΩ resistor | 3 | Button pull-ups (if no INPUT_PULLUP) |
| USB power supply | 1 | 5V power |

---

## 🔌 Wiring Diagrams

### Receiver (Display Unit)

```
ESP32           SSD1306 OLED
─────           ────────────
GPIO 21  ──────  SDA
GPIO 22  ──────  SCL
3.3V     ──────  VCC
GND      ──────  GND

ESP32           Buttons (active LOW, INPUT_PULLUP)
─────           ──────────────────────────────────
GPIO 25  ──[BTN_PREV]── GND
GPIO 26  ──[BTN_NEXT]── GND
GPIO 27  ──[BTN_SET]──  GND
```

### Transmitter (Sensor Node)

```
ESP32           INA226
─────           ──────
GPIO 21  ──────  SDA
GPIO 22  ──────  SCL
3.3V     ──────  VCC
GND      ──────  GND

                Panel wiring:
                + ──[shunt 10mΩ]── INA226 V+ ── to load
                                   INA226 V- ── from load
                                   INA226 VBS ── panel +

ESP32           DS18B20
─────           ───────
GPIO 5   ──────  DATA (with 4.7kΩ to 3.3V)
3.3V     ──────  VCC
GND      ──────  GND

ESP32           DHT22
─────           ─────
GPIO 4   ──────  DATA
3.3V     ──────  VCC
GND      ──────  GND

ESP32           BH1750
─────           ──────
GPIO 21  ──────  SDA (shared I2C bus)
GPIO 22  ──────  SCL
3.3V     ──────  VCC
GND      ──────  GND

ESP32           GP2Y1010
─────           ────────
GPIO 34  ──────  AOUT (analog output)
GPIO 16  ──────  ILED (LED control, active LOW)
3.3V     ──────  VCC (through 150Ω)
GND      ──────  GND (with 220µF cap across VCC-GND)
```

---

## 📟 Display Pages (11 screens)

Navigate with **PREV** ◄ and **NEXT** ► buttons.

| # | Page | Content |
|---|------|---------|
| 1 | **Overview** | All vitals at a glance + active alerts |
| 2 | **Voltage** | Large voltage + range bar |
| 3 | **Current** | Current draw in Amperes |
| 4 | **Power** | Watt output + efficiency proxy |
| 5 | **Temperature** | Panel + ambient temps |
| 6 | **Dust Index** | Soiling % + progress bar |
| 7 | **Uptime** | Node uptime + RSSI signal strength |
| 8 | **Wash Alert** | Wash recommendation + estimated power loss |
| 9 | **Set V-Low** | Configure under-voltage alert threshold |
| 10 | **Set V-High** | Configure over-voltage alert threshold |
| 11 | **Set Dust Thresh** | Configure wash-trigger dust level |

### Using SET Mode (pages 9–11)

1. Navigate to a settings page
2. Press **SET** to enter adjustment mode (arrows shown)
3. Press **PREV** to decrease value, **NEXT** to increase
4. Press **SET** again to confirm — saved to flash (persists reboot)

---

## 📦 Required Libraries

Install via Arduino Library Manager or PlatformIO:

| Library | Author | Purpose |
|---------|--------|---------|
| `Adafruit SSD1306` | Adafruit | OLED display driver |
| `Adafruit GFX Library` | Adafruit | Graphics primitives |
| `DallasTemperature` | Miles Burton | DS18B20 temperature |
| `OneWire` | Paul Stoffregen | 1-Wire bus |
| `DHT sensor library` | Adafruit | DHT22 temp/humidity |
| `BH1750` | Christopher Laws | Light sensor |
| `INA226` | Rob Tillaart | Voltage/current sensor |

---

## 🚀 Setup & Flashing

**Build-layout limitation:** `platformio.ini` filters for `solar_monitor.ino` and `transmitter/transmitter.ino` under PlatformIO's default `src/` directory, but the committed sketches are `solar_monitor.ino` and `transmitter.ino` at the repository root. The PlatformIO build/upload commands are therefore not verified and are not expected to work against this tree without a separate layout/configuration correction. No source files were moved in this maintenance pass.

**Arduino IDE staging:** The repository root contains two different `.ino` files, not a single Arduino sketch. In a separate local working copy, create `solar_monitor/solar_monitor.ino` by copying the root `solar_monitor.ino`, and create `transmitter/transmitter.ino` by copying the root `transmitter.ino`. Open and build each sketch from its own matching folder, and install the libraries listed in `LIBRARIES.md`. These are local staging copies only; no tracked source file is moved by this guide.

### Step 1: Find Receiver MAC address
Flash this sketch to the **receiver** ESP32 first and read Serial Monitor:
```cpp
#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
}
void loop() {}
```

### Step 2: Update transmitter with MAC
Open `transmitter.ino` and replace:
```cpp
uint8_t receiverMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
```
With your receiver's MAC, e.g.:
```cpp
uint8_t receiverMAC[] = {0xA4, 0xCF, 0x12, 0x34, 0x56, 0x78};
```

### Step 3: Flash both boards
1. Open the local `solar_monitor/solar_monitor.ino` copy and flash it to the receiver/display ESP32.
2. Open the local `transmitter/transmitter.ino` copy and flash it to the sensor-node ESP32.

### Step 4: Power up & enjoy!
The receiver will show "Waiting for node..." until the first packet arrives.

---

## 🔧 Calibration

### INA226 Shunt Calibration
Adjust `SHUNT_OHM` and `MAX_CURRENT_A` in `transmitter.ino` to match your shunt resistor:
```cpp
const float SHUNT_OHM     = 0.01f;   // 10 mΩ for 20A panels
const float MAX_CURRENT_A  = 20.0f;
```

### Dust Sensor Calibration
The `dustVoltToPercent()` function uses an empirical formula. For best accuracy:
1. Measure in a clean environment → set that voltage as 0%
2. Measure in a dusty environment → set that as 100%
3. Adjust the formula constants accordingly

---

## 📡 Multi-Panel Support

The current receiver stores a single `SolarData` packet in `rxData`; it does not maintain a table or carousel of separate nodes. `NODE_ID` is transmitted and displayed, but multiple transmitters are not independently tracked by the current receiver UI. Treat this as a single-node prototype until multi-node storage, selection, and testing are implemented. The referenced `docs/MULTI_NODE.md` file is not present in the checked-in repository.

---

## ⚡ Power Consumption

| Mode | Current |
|------|---------|
| Transmitter active | ~160 mA |
| Transmitter light sleep | ~2 mA |
| Receiver + OLED | ~90 mA |

Self-powered transmitter: a 5W solar panel + 3000mAh 18650 will run continuously.

---

## Known Limitations

- `receiverMAC` in `transmitter.ino` is a placeholder of all `0xFF` bytes; configure the intended receiver address before relying on delivery.
- The PlatformIO source paths do not match the root-level sketch files, as noted above.
- The soiling percentage is based on an empirical formula and is not a calibrated measurement of panel dirt or lost energy.
- The display's power/irradiance-derived value is only a rough proxy, not a validated panel-efficiency calculation.
- The DHT22 humidity value is read but is not part of the transmitted `SolarData` structure.
- No firmware build or physical sensor/link test was performed as part of this documentation review.

---

## 📜 License

MIT License — free for personal and commercial use. Attribution appreciated!

---

## 🤝 Contributing

Pull requests welcome! Please open an issue first to discuss changes.

```
git clone https://github.com/yourusername/solar-monitor.git
cd solar-monitor
```

---

*Built with ❤️ for renewable energy enthusiasts*


---

## Skills & technologies

`Arduino/C++` · `HTML/CSS`



## Maintenance & Audit

- **Audit date:** 18 November 2026
- **Documentation status:** README reviewed and audit record added on this date.
- **Scope and evidence:** This is a documentation-level maintenance pass. It does not establish that source code compiles, dependencies are current, security has been tested, or hardware/runtime behavior has been verified.
- **Code and tests:** No new functional code changes or test results are claimed by this README update. Run project-specific build, test, and hardware/runtime checks before relying on the project.
- **Maintenance baseline:** Documentation status recorded through 11 November 2026; revisit setup instructions, dependencies, known limitations, and tests when the project changes.
