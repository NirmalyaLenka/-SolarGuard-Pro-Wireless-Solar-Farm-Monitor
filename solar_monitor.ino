/*
 * ============================================================
 * SolarGuard Pro - Solar Farm Monitoring System
 * ============================================================
 * Hardware: ESP32 (Transmitter) + ESP32 (Receiver/Display)
 * Protocol: ESP-NOW (Peer-to-peer, no Wi-Fi router needed)
 * Display:  0.96" or 1.3" SSD1306/SH1106 OLED (128x64)
 * Sensors:  INA226 (V/I), DS18B20 (Temp), Dust/Soil Moisture
 *
 * Buttons:
 *   BTN_PREV  (GPIO 25) - Previous screen
 *   BTN_NEXT  (GPIO 26) - Next screen
 *   BTN_SET   (GPIO 27) - Enter set mode / confirm value
 *
 * Author: SolarGuard Team
 * License: MIT
 * Version: 2.0.0
 * ============================================================
 */

// ──────────────────────────────────────────────
//  This file is the RECEIVER / DISPLAY unit.
//  See transmitter/transmitter.ino for the
//  sensor node that runs on the solar panel.
// ──────────────────────────────────────────────

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>       // NVS flash storage
#include <time.h>

// ── OLED ──────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDR    0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ── BUTTONS ───────────────────────────────────
#define BTN_PREV  25   // Previous page / decrement
#define BTN_NEXT  26   // Next page / increment
#define BTN_SET   27   // Set / confirm

// Debounce
#define DEBOUNCE_MS 200
volatile unsigned long lastPrev = 0, lastNext = 0, lastSet = 0;

// ── SCREEN PAGES ──────────────────────────────
enum Page {
  PAGE_OVERVIEW = 0,
  PAGE_VOLTAGE,
  PAGE_CURRENT,
  PAGE_POWER,
  PAGE_TEMPERATURE,
  PAGE_DUST,
  PAGE_UPTIME,
  PAGE_WASH_ALERT,
  PAGE_SET_VOLT_LOW,
  PAGE_SET_VOLT_HIGH,
  PAGE_SET_DUST_THRESH,
  PAGE_TOTAL
};

const char* PAGE_NAMES[] = {
  "OVERVIEW",
  "VOLTAGE",
  "CURRENT",
  "POWER",
  "TEMPERATURE",
  "DUST INDEX",
  "UPTIME",
  "WASH ALERT",
  "SET V-LOW",
  "SET V-HIGH",
  "SET DUST LVL"
};

uint8_t currentPage = PAGE_OVERVIEW;
bool    inSetMode   = false;

// ── PERSISTENT SETTINGS (NVS) ─────────────────
Preferences prefs;
float  setVoltLow   = 18.0f;   // Under-voltage alert threshold
float  setVoltHigh  = 24.0f;   // Over-voltage alert threshold
float  setDustThresh = 60.0f;  // Dust % above which washing is recommended
float  setVoltStep  = 0.5f;    // Adjustment step for voltage set pages

// ── TELEMETRY PACKET (shared with transmitter) ─
typedef struct SolarData {
  float voltage;          // Panel voltage (V)
  float current;          // Panel current (A)
  float power;            // Computed power (W)
  float panelTemp;        // Panel surface temperature (°C)
  float ambientTemp;      // Ambient air temperature (°C)
  float dustLevel;        // Dust/soiling index 0–100 %
  float irradiance;       // Light irradiance proxy (lux)
  uint32_t uptimeSeconds; // Panel node uptime in seconds
  uint8_t  nodeID;        // Panel ID (multi-panel support)
  bool     alertVoltLow;
  bool     alertVoltHigh;
  bool     alertOverTemp;
  uint8_t  rssi;          // Signal strength (filled at receiver)
} SolarData;

SolarData rxData;
bool      dataReceived  = false;
uint32_t  lastRxMillis  = 0;
#define   DATA_TIMEOUT_MS 10000  // 10 s without data = offline

// ── UPTIME DISPLAY ────────────────────────────
String formatUptime(uint32_t sec) {
  uint32_t d = sec / 86400;
  uint32_t h = (sec % 86400) / 3600;
  uint32_t m = (sec % 3600)  / 60;
  uint32_t s = sec % 60;
  char buf[32];
  if (d > 0) snprintf(buf, sizeof(buf), "%dd %02dh %02dm", d, h, m);
  else        snprintf(buf, sizeof(buf), "%02dh %02dm %02ds", h, m, s);
  return String(buf);
}

// ── ESP-NOW CALLBACK ──────────────────────────
void onDataReceive(const esp_now_recv_info_t* info, const uint8_t* inData, int len) {
  if (len == sizeof(SolarData)) {
    memcpy(&rxData, inData, sizeof(SolarData));
    rxData.rssi     = info->rx_ctrl->rssi * -1; // Make positive
    dataReceived    = true;
    lastRxMillis    = millis();
  }
}

// ── BUTTON ISRs ───────────────────────────────
volatile bool btnPrevPressed = false;
volatile bool btnNextPressed = false;
volatile bool btnSetPressed  = false;

void IRAM_ATTR isrPrev() {
  if (millis() - lastPrev > DEBOUNCE_MS) { btnPrevPressed = true; lastPrev = millis(); }
}
void IRAM_ATTR isrNext() {
  if (millis() - lastNext > DEBOUNCE_MS) { btnNextPressed = true; lastNext = millis(); }
}
void IRAM_ATTR isrSet() {
  if (millis() - lastSet  > DEBOUNCE_MS) { btnSetPressed  = true; lastSet  = millis(); }
}

// ── DRAWING HELPERS ───────────────────────────
void drawHeader(const char* title, bool offline = false) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(title);
  if (offline) {
    display.setCursor(90, 0);
    display.print("OFFLINE");
  } else {
    display.setCursor(104, 0);
    char rssiStr[8];
    snprintf(rssiStr, sizeof(rssiStr), "-%d", rxData.rssi);
    display.print(rssiStr);
  }
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
}

void drawBigValue(float val, const char* unit, int y = 20) {
  display.setTextSize(2);
  display.setCursor(0, y);
  display.print(val, 2);
  display.setTextSize(1);
  display.print(" ");
  display.print(unit);
}

void drawProgressBar(int x, int y, int w, int h, float pct) {
  display.drawRect(x, y, w, h, SSD1306_WHITE);
  int fill = (int)((pct / 100.0f) * (w - 2));
  fill = constrain(fill, 0, w - 2);
  display.fillRect(x + 1, y + 1, fill, h - 2, SSD1306_WHITE);
}

void drawAlert(const char* msg) {
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.setTextColor(SSD1306_WHITE);
  display.print("! ");
  display.print(msg);
}

// ── PAGE RENDERERS ────────────────────────────

void renderOverview() {
  bool offline = (millis() - lastRxMillis > DATA_TIMEOUT_MS);
  drawHeader("SOLAR OVERVIEW", offline);

  if (offline) {
    display.setTextSize(1);
    display.setCursor(20, 28);
    display.print("Waiting for node...");
    return;
  }

  display.setTextSize(1);
  // Row 1: Voltage + Current
  display.setCursor(0, 12);  display.print("V:");
  display.setCursor(12, 12); display.print(rxData.voltage, 1); display.print("V");
  display.setCursor(64, 12); display.print("I:");
  display.setCursor(76, 12); display.print(rxData.current, 2); display.print("A");

  // Row 2: Power + Temp
  display.setCursor(0, 22);  display.print("P:");
  display.setCursor(12, 22); display.print(rxData.power, 1); display.print("W");
  display.setCursor(64, 22); display.print("T:");
  display.setCursor(76, 22); display.print(rxData.panelTemp, 1); display.print("C");

  // Row 3: Dust bar
  display.setCursor(0, 34); display.print("Dust:");
  drawProgressBar(34, 34, 60, 8, rxData.dustLevel);
  display.setCursor(96, 34); display.print((int)rxData.dustLevel); display.print("%");

  // Row 4: Uptime
  display.setCursor(0, 46); display.print("Up:");
  display.setCursor(20, 46); display.print(formatUptime(rxData.uptimeSeconds));

  // Alerts
  if (rxData.alertVoltLow)   drawAlert("LOW VOLT");
  else if (rxData.alertVoltHigh)  drawAlert("HIGH VOLT");
  else if (rxData.alertOverTemp)  drawAlert("OVER TEMP");
  else if (rxData.dustLevel >= setDustThresh) drawAlert("WASH PANEL");
}

void renderVoltage() {
  bool offline = (millis() - lastRxMillis > DATA_TIMEOUT_MS);
  drawHeader("VOLTAGE", offline);
  drawBigValue(rxData.voltage, "V", 16);

  display.setTextSize(1);
  display.setCursor(0, 38); display.print("Min: "); display.print(setVoltLow, 1); display.print(" V");
  display.setCursor(0, 48); display.print("Max: "); display.print(setVoltHigh, 1); display.print(" V");

  // Mini bar showing where voltage sits in range
  float pct = 0;
  float range = setVoltHigh - setVoltLow;
  if (range > 0) pct = ((rxData.voltage - setVoltLow) / range) * 100.0f;
  pct = constrain(pct, 0, 100);
  drawProgressBar(0, 57, 128, 7, pct);
}

void renderCurrent() {
  drawHeader("CURRENT");
  drawBigValue(rxData.current, "A", 16);
  display.setTextSize(1);
  display.setCursor(0, 40); display.print("Irrad: "); display.print(rxData.irradiance, 0); display.print(" lux");
}

void renderPower() {
  drawHeader("POWER OUTPUT");
  drawBigValue(rxData.power, "W", 16);
  display.setTextSize(1);
  display.setCursor(0, 40); display.print("Efficiency proxy:");
  float eff = (rxData.irradiance > 0) ? (rxData.power / rxData.irradiance) * 100.0f : 0;
  display.setCursor(0, 50); display.print(eff, 3); display.print(" W/lux");
}

void renderTemperature() {
  drawHeader("TEMPERATURE");
  drawBigValue(rxData.panelTemp, "°C", 14);
  display.setTextSize(1);
  display.setCursor(0, 40); display.print("Ambient: "); display.print(rxData.ambientTemp, 1); display.print(" C");
  display.setCursor(0, 50);
  if (rxData.panelTemp > 65) display.print("STATUS: OVER TEMP!");
  else if (rxData.panelTemp > 50) display.print("STATUS: HOT");
  else display.print("STATUS: NORMAL");
}

void renderDust() {
  drawHeader("DUST / SOILING");
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print((int)rxData.dustLevel);
  display.print("%");
  display.setTextSize(1);
  display.setCursor(50, 20);
  if (rxData.dustLevel < 30)       display.print("CLEAN");
  else if (rxData.dustLevel < 60)  display.print("MODERATE");
  else                             display.print("DIRTY-WASH!");

  drawProgressBar(0, 36, 128, 10, rxData.dustLevel);

  display.setCursor(0, 50); display.print("Thresh: "); display.print(setDustThresh, 0); display.print("%");
}

void renderUptime() {
  drawHeader("NODE UPTIME");
  display.setTextSize(1);
  display.setCursor(0, 14);
  display.print("Panel ID: #"); display.print(rxData.nodeID);
  display.setCursor(0, 26);
  display.print(formatUptime(rxData.uptimeSeconds));
  display.setCursor(0, 40);
  display.print("RSSI: -"); display.print(rxData.rssi); display.print(" dBm");
  display.setCursor(0, 52);
  display.print("Link: ");
  display.print(rxData.rssi < 70 ? "STRONG" : rxData.rssi < 85 ? "FAIR" : "WEAK");
}

void renderWashAlert() {
  drawHeader("WASH SCHEDULE");
  display.setTextSize(1);
  display.setCursor(0, 12);
  float daysSinceClean = rxData.uptimeSeconds / 86400.0f; // Simplification; real impl uses RTC
  display.print("Dust idx: "); display.print(rxData.dustLevel, 1); display.print("%");
  display.setCursor(0, 24);
  display.print("Threshold: "); display.print(setDustThresh, 0); display.print("%");
  display.setCursor(0, 36);
  if (rxData.dustLevel >= setDustThresh) {
    display.print("** WASH NOW **");
    display.setCursor(0, 48);
    display.print("Est loss: ~"); display.print(rxData.dustLevel * 0.3f, 1); display.print("%");
  } else {
    float remaining = setDustThresh - rxData.dustLevel;
    display.print("OK - "); display.print(remaining, 1); display.print("% to thresh");
    display.setCursor(0, 48);
    display.print("Est days: "); display.print((int)(remaining / 2)); // 2% dust/day assumption
  }
}

void renderSetVoltLow() {
  drawHeader("SET V-LOW ALERT");
  display.setTextSize(2);
  display.setCursor(20, 18);
  display.print(setVoltLow, 1);
  display.print(" V");
  display.setTextSize(1);
  if (inSetMode) {
    display.setCursor(0, 50);
    display.print("<- PREV=Dec  NEXT=Inc ->");
  } else {
    display.setCursor(10, 50);
    display.print("Press SET to adjust");
  }
}

void renderSetVoltHigh() {
  drawHeader("SET V-HIGH ALERT");
  display.setTextSize(2);
  display.setCursor(20, 18);
  display.print(setVoltHigh, 1);
  display.print(" V");
  display.setTextSize(1);
  if (inSetMode) {
    display.setCursor(0, 50);
    display.print("<- PREV=Dec  NEXT=Inc ->");
  } else {
    display.setCursor(10, 50);
    display.print("Press SET to adjust");
  }
}

void renderSetDustThresh() {
  drawHeader("SET DUST THRESH");
  display.setTextSize(2);
  display.setCursor(20, 18);
  display.print((int)setDustThresh);
  display.print(" %");
  display.setTextSize(1);
  if (inSetMode) {
    display.setCursor(0, 50);
    display.print("<- PREV=Dec  NEXT=Inc ->");
  } else {
    display.setCursor(10, 50);
    display.print("Press SET to adjust");
  }
}

// ── SAVE / LOAD SETTINGS ──────────────────────
void saveSettings() {
  prefs.begin("solar", false);
  prefs.putFloat("vLow",  setVoltLow);
  prefs.putFloat("vHigh", setVoltHigh);
  prefs.putFloat("dust",  setDustThresh);
  prefs.end();
}

void loadSettings() {
  prefs.begin("solar", true);
  setVoltLow   = prefs.getFloat("vLow",  18.0f);
  setVoltHigh  = prefs.getFloat("vHigh", 24.0f);
  setDustThresh= prefs.getFloat("dust",  60.0f);
  prefs.end();
}

// ── BUTTON HANDLER ────────────────────────────
void handleButtons() {
  bool isSetPage = (currentPage == PAGE_SET_VOLT_LOW ||
                    currentPage == PAGE_SET_VOLT_HIGH ||
                    currentPage == PAGE_SET_DUST_THRESH);

  if (btnSetPressed) {
    btnSetPressed = false;
    if (isSetPage) {
      inSetMode = !inSetMode;
      if (!inSetMode) saveSettings(); // Save on exit
    }
  }

  if (btnPrevPressed) {
    btnPrevPressed = false;
    if (inSetMode && isSetPage) {
      if (currentPage == PAGE_SET_VOLT_LOW)    setVoltLow   -= setVoltStep;
      if (currentPage == PAGE_SET_VOLT_HIGH)   setVoltHigh  -= setVoltStep;
      if (currentPage == PAGE_SET_DUST_THRESH) setDustThresh -= 5.0f;
    } else {
      currentPage = (currentPage == 0) ? PAGE_TOTAL - 1 : currentPage - 1;
      inSetMode = false;
    }
  }

  if (btnNextPressed) {
    btnNextPressed = false;
    if (inSetMode && isSetPage) {
      if (currentPage == PAGE_SET_VOLT_LOW)    setVoltLow   += setVoltStep;
      if (currentPage == PAGE_SET_VOLT_HIGH)   setVoltHigh  += setVoltStep;
      if (currentPage == PAGE_SET_DUST_THRESH) setDustThresh += 5.0f;
    } else {
      currentPage = (currentPage + 1) % PAGE_TOTAL;
      inSetMode = false;
    }
  }

  // Clamp values
  setVoltLow    = constrain(setVoltLow,    0.0f, 100.0f);
  setVoltHigh   = constrain(setVoltHigh,   0.0f, 100.0f);
  setDustThresh = constrain(setDustThresh, 10.0f, 100.0f);
}

// ── RENDER DISPATCHER ─────────────────────────
void renderPage() {
  display.clearDisplay();
  switch (currentPage) {
    case PAGE_OVERVIEW:       renderOverview();      break;
    case PAGE_VOLTAGE:        renderVoltage();       break;
    case PAGE_CURRENT:        renderCurrent();       break;
    case PAGE_POWER:          renderPower();         break;
    case PAGE_TEMPERATURE:    renderTemperature();   break;
    case PAGE_DUST:           renderDust();          break;
    case PAGE_UPTIME:         renderUptime();        break;
    case PAGE_WASH_ALERT:     renderWashAlert();     break;
    case PAGE_SET_VOLT_LOW:   renderSetVoltLow();    break;
    case PAGE_SET_VOLT_HIGH:  renderSetVoltHigh();   break;
    case PAGE_SET_DUST_THRESH:renderSetDustThresh(); break;
  }
  // Page indicator dots at bottom right
  for (int i = 0; i < PAGE_TOTAL; i++) {
    int x = 128 - PAGE_TOTAL * 5 + i * 5;
    if (i == currentPage) display.fillRect(x, 62, 3, 2, SSD1306_WHITE);
    else                  display.drawRect(x, 62, 3, 2, SSD1306_WHITE);
  }
  display.display();
}

// ── SPLASH SCREEN ─────────────────────────────
void showSplash() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 8);
  display.print("SolarGuard");
  display.setTextSize(1);
  display.setCursor(30, 30);
  display.print("Pro v2.0");
  display.setCursor(15, 44);
  display.print("Wireless Monitor");
  display.setCursor(28, 56);
  display.print("Initializing...");
  display.display();
  delay(2500);
}

// ── SETUP ─────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // OLED init
  Wire.begin(21, 22); // SDA=21, SCL=22 (ESP32 default)
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed!");
    while (true);
  }
  showSplash();

  // Load saved settings
  loadSettings();

  // Buttons
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_SET,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_PREV), isrPrev, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_NEXT), isrNext, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_SET),  isrSet,  FALLING);

  // ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(onDataReceive);

  Serial.println("SolarGuard receiver ready.");
  Serial.print("MAC: "); Serial.println(WiFi.macAddress());
}

// ── LOOP ──────────────────────────────────────
uint32_t lastRender = 0;
#define RENDER_INTERVAL_MS 250

void loop() {
  handleButtons();

  if (millis() - lastRender > RENDER_INTERVAL_MS) {
    renderPage();
    lastRender = millis();
  }
}
