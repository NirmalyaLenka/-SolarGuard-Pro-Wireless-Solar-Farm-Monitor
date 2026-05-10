# Troubleshooting & Calibration Guide

## 🔍 Common Issues

### OLED shows nothing
- Check I2C address: default is `0x3C`. Run an I2C scanner sketch to confirm.
- Verify SDA/SCL wiring (GPIO 21/22 on ESP32).
- Try 3.3V or 5V for OLED VCC depending on your module.

### "Waiting for node..." never goes away
1. Confirm both ESP32s are on the **same Wi-Fi channel** (default 0 = auto).
2. Double-check the MAC address in `transmitter.ino` matches the receiver's `WiFi.macAddress()`.
3. Ensure `esp_now_init()` succeeds on both devices (check Serial Monitor).
4. Move the transmitter closer during initial test.

### Voltage reads 0 or wrong
- Verify INA226 I2C address (A0/A1 pins on the module).
- Check shunt wiring polarity: V+ is panel side, V- is load side.
- Adjust `SHUNT_OHM` to match your physical shunt resistor.

### Temperature reads -127°C (DS18B20)
- The sensor is disconnected or the 4.7kΩ pull-up is missing.
- Run `ds.getAddress()` to verify the sensor is found on the bus.

### Dust sensor always reads 0% or 100%
- Check GP2Y1010 wiring — the LED control pin matters for timing.
- Add 220µF cap across VCC-GND right at the sensor.
- Calibrate: blow dust near sensor and observe ADC values in Serial Monitor.

### Buttons don't respond
- Confirm `INPUT_PULLUP` is set for each button pin.
- Check buttons are wired from GPIO to GND (not VCC).
- Increase `DEBOUNCE_MS` if buttons trigger multiple times.

---

## 🔧 Sensor Calibration

### INA226 Voltage Calibration
1. Measure actual panel voltage with a trusted multimeter.
2. Compare to `ina226.getBusVoltage()` in Serial Monitor.
3. If offset exists, add a correction factor:
   ```cpp
   txData.voltage = ina226.getBusVoltage() * 1.02f; // example: +2% correction
   ```

### Dust Sensor Calibration
1. In clean air: note ADC value (voltage_mV) — this is your "0% dust" baseline.
2. In dusty environment (e.g., blow dust across sensor): note "100% dust" voltage.
3. Update the formula in `dustVoltToPercent()`:
   ```cpp
   float dustVoltToPercent(float v_mV) {
     float pct = (v_mV - YOUR_CLEAN_MV) / (YOUR_DIRTY_MV - YOUR_CLEAN_MV) * 100.0f;
     return constrain(pct, 0, 100);
   }
   ```

### BH1750 Verification
Compare readings to a reference lux meter at noon direct sunlight (≈100,000 lux).
The BH1750 saturates at ~65535 lux; for high irradiance, use `ONE_TIME_HIGH_RES_MODE_2`.

---

## 🔄 Factory Reset

To reset all saved thresholds to defaults:
1. Hold **SET** button while powering on the receiver.
2. The OLED will display "RESET TO DEFAULTS" for 2 seconds.
3. Release button — defaults are restored and saved.

*(Add this to `setup()` if desired — check `digitalRead(BTN_SET) == LOW` before loading prefs)*

---

## 📊 Serial Monitor Diagnostics

Receiver serial output (115200 baud):
```
SolarGuard receiver ready.
MAC: A4:CF:12:34:56:78
```

Transmitter serial output:
```
SolarGuard transmitter ready.
[Node 1] V=20.12V I=4.230A P=85.11W Tp=48.3C Ta=32.1C Lux=75420 Dust=23.4% Up=3600s
TX Status: OK
```
