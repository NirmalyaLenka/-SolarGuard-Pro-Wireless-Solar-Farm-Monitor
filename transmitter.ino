/*
 * ============================================================
 * SolarGuard Pro - TRANSMITTER / SENSOR NODE
 * ============================================================
 * Runs on: ESP32 mounted at the solar panel
 * Sends data wirelessly via ESP-NOW to the display unit
 *
 * Sensors:
 *  ┌─────────────────────────────────────────────────────┐
 *  │ INA226  (I2C 0x40)  – Voltage + Current (High-side) │
 *  │ DS18B20 (OneWire)   – Panel surface temperature      │
 *  │ DHT22   (GPIO 4)    – Ambient Temp + Humidity        │
 *  │ BH1750  (I2C 0x23)  – Light / Irradiance (lux)      │
 *  │ GP2Y1010 or TSL2561 – Dust / Soiling index           │
 *  └─────────────────────────────────────────────────────┘
 *
 * Wiring (ESP32):
 *  INA226   SDA → GPIO 21 | SCL → GPIO 22
 *  BH1750   SDA → GPIO 21 | SCL → GPIO 22
 *  DS18B20  DATA → GPIO 5 (with 4.7kΩ pull-up to 3.3V)
 *  DHT22    DATA → GPIO 4
 *  GP2Y1010 AOUT → GPIO 34 (ADC1 CH6)
 *           ILED → GPIO 16 (LED control)
 *
 * Power: Solar + LiPo + TP4056; ESP32 in light sleep between sends
 * ============================================================
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <BH1750.h>
#include "INA226.h"           // Rob Tillaart's INA226 library

// ── CONFIG ────────────────────────────────────
// !! Replace with your RECEIVER's MAC address !!
uint8_t receiverMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define NODE_ID         1        // Unique ID for this panel
#define SEND_INTERVAL   5000     // Send every 5 seconds (ms)

// ── PINS ──────────────────────────────────────
#define ONE_WIRE_BUS    5
#define DHT_PIN         4
#define DHT_TYPE        DHT22
#define DUST_AOUT_PIN   34       // ADC pin for GP2Y1010
#define DUST_LED_PIN    16       // IR LED control pin for GP2Y1010

// ── SENSOR OBJECTS ────────────────────────────
OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature dsSensors(&oneWire);
DHT               dht(DHT_PIN, DHT_TYPE);
BH1750            lightMeter;
INA226            ina226;

// ── TELEMETRY PACKET ─────────────────────────
typedef struct SolarData {
  float voltage;
  float current;
  float power;
  float panelTemp;
  float ambientTemp;
  float dustLevel;
  float irradiance;
  uint32_t uptimeSeconds;
  uint8_t  nodeID;
  bool     alertVoltLow;
  bool     alertVoltHigh;
  bool     alertOverTemp;
  uint8_t  rssi;
} SolarData;

SolarData txData;

// ── ALERT THRESHOLDS ─────────────────────────
const float VOLT_LOW_THRESH   = 18.0f;   // V
const float VOLT_HIGH_THRESH  = 24.0f;   // V
const float TEMP_HIGH_THRESH  = 70.0f;   // °C (panel)

// ── CALIBRATION ──────────────────────────────
// INA226: set shunt resistor value (Ω) matching your hardware
// e.g. 0.01 Ω for high-current panels, 0.1 Ω for smaller ones
const float SHUNT_OHM        = 0.01f;
const float MAX_CURRENT_A    = 20.0f;    // Adjust per panel rating

// GP2Y1010 Dust: empirical formula; tweak for your environment
float dustVoltToPercent(float v_mV) {
  // Formula: dust density (ug/m3) ≈ (v_mV - 0.6) / 5 * 100
  // We normalize to 0–100% soiling index
  float density = (v_mV - 600.0f) / 5.0f;
  density = constrain(density, 0, 100);
  return density;
}

// ── ESP-NOW SEND CALLBACK ─────────────────────
void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  Serial.print("TX Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// ── READ DUST SENSOR (GP2Y1010) ───────────────
float readDustPercent() {
  digitalWrite(DUST_LED_PIN, LOW);   // Turn on IR LED (active low)
  delayMicroseconds(280);            // Wait 280µs before sampling
  int raw = analogRead(DUST_AOUT_PIN);
  delayMicroseconds(40);
  digitalWrite(DUST_LED_PIN, HIGH);  // Turn off IR LED
  delayMicroseconds(9680);           // Rest of duty cycle

  // ESP32 ADC: 12-bit (0-4095), 3.3V ref
  float voltage_mV = (raw / 4095.0f) * 3300.0f;
  return dustVoltToPercent(voltage_mV);
}

// ── READ ALL SENSORS ──────────────────────────
void readAllSensors() {
  // --- INA226: Voltage & Current ---
  txData.voltage = ina226.getBusVoltage();   // Volts
  txData.current = ina226.getCurrent_mA() / 1000.0f; // A
  txData.power   = txData.voltage * txData.current;   // W

  // Guard against negative (dark / disconnected)
  if (txData.current < 0) txData.current = 0;
  if (txData.power   < 0) txData.power   = 0;

  // --- DS18B20: Panel surface temp ---
  dsSensors.requestTemperatures();
  txData.panelTemp = dsSensors.getTempCByIndex(0);
  if (txData.panelTemp == DEVICE_DISCONNECTED_C) txData.panelTemp = -99;

  // --- DHT22: Ambient temperature ---
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  txData.ambientTemp = isnan(t) ? 25.0f : t;

  // --- BH1750: Light irradiance ---
  txData.irradiance = lightMeter.readLightLevel();
  if (txData.irradiance < 0) txData.irradiance = 0;

  // --- GP2Y1010: Dust / soiling ---
  // Average 5 samples for stability
  float dustSum = 0;
  for (int i = 0; i < 5; i++) {
    dustSum += readDustPercent();
    delay(10);
  }
  txData.dustLevel = dustSum / 5.0f;

  // --- Metadata ---
  txData.uptimeSeconds = millis() / 1000;
  txData.nodeID        = NODE_ID;

  // --- Alerts ---
  txData.alertVoltLow  = (txData.voltage < VOLT_LOW_THRESH  && txData.voltage > 0.5f);
  txData.alertVoltHigh = (txData.voltage > VOLT_HIGH_THRESH);
  txData.alertOverTemp = (txData.panelTemp > TEMP_HIGH_THRESH);

  Serial.printf("[Node %d] V=%.2fV I=%.3fA P=%.2fW Tp=%.1fC Ta=%.1fC Lux=%.0f Dust=%.1f%% Up=%us\n",
    NODE_ID, txData.voltage, txData.current, txData.power,
    txData.panelTemp, txData.ambientTemp, txData.irradiance,
    txData.dustLevel, txData.uptimeSeconds);
}

// ── SETUP ─────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Pins
  pinMode(DUST_LED_PIN, OUTPUT);
  digitalWrite(DUST_LED_PIN, HIGH); // Off by default (active low)

  // I2C sensors
  Wire.begin(21, 22);

  // INA226
  ina226.begin(0x40);
  ina226.configure(INA226_AVERAGES_16, INA226_BUS_CONV_TIME_1100US,
                   INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);
  ina226.calibrate(SHUNT_OHM, MAX_CURRENT_A);

  // DS18B20
  dsSensors.begin();

  // DHT22
  dht.begin();

  // BH1750
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 not found, using 0 lux");
  }

  // ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!"); return;
  }
  esp_now_register_send_cb(onDataSent);

  // Register peer (receiver)
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("SolarGuard transmitter ready.");
  Serial.print("Receiver MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", receiverMAC[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

// ── LOOP ──────────────────────────────────────
uint32_t lastSend = 0;

void loop() {
  if (millis() - lastSend > SEND_INTERVAL) {
    readAllSensors();
    esp_err_t result = esp_now_send(receiverMAC, (uint8_t*)&txData, sizeof(txData));
    if (result != ESP_OK) Serial.println("ESP-NOW send error");
    lastSend = millis();
  }
  // Optionally: put ESP32 in light sleep between sends to save power
  // esp_sleep_enable_timer_wakeup(SEND_INTERVAL * 1000); // microseconds
  // esp_light_sleep_start();
}
