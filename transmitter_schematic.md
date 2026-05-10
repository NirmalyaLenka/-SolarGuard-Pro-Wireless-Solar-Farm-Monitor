# Transmitter (Sensor Node) — Full Wiring Schematic

## Block Diagram

```
                        ┌─────────────────────────────────────────┐
                        │           ESP32 DevKit V1                │
                        │                                          │
  INA226 ───── SDA ────►│ GPIO 21 (SDA)                            │
  INA226 ───── SCL ────►│ GPIO 22 (SCL)                            │
  BH1750 ───── SDA ────►│ GPIO 21 (shared I2C)                     │
  BH1750 ───── SCL ────►│ GPIO 22 (shared I2C)                     │
                        │                                          │
  DS18B20 DATA ────────►│ GPIO 5  (OneWire)                        │
                        │                                          │
  DHT22 DATA ──────────►│ GPIO 4  (DHT)                            │
                        │                                          │
  GP2Y1010 AOUT ───────►│ GPIO 34 (ADC1 CH6)                       │
  GP2Y1010 ILED ───────►│ GPIO 16 (Digital Out)                    │
                        │                                          │
         3.3V ──────────│ 3.3V                                     │
         GND ───────────│ GND                                      │
                        └─────────────────────────────────────────┘
```

## INA226 – High-side Current/Voltage Monitor

```
Solar Panel (+) ──────────────── INA226 (V+/IN+)
                                         │
                                    [10mΩ Shunt]
                                         │
Solar Panel Load (+) ─────────── INA226 (V-/IN-)

INA226 VBS ──── Solar Panel (+)  [bus voltage reference]
INA226 SDA ──── ESP32 GPIO 21
INA226 SCL ──── ESP32 GPIO 22
INA226 VCC ──── 3.3V
INA226 GND ──── GND
INA226 A0  ──── GND  [I2C address 0x40]
INA226 A1  ──── GND
```

## DS18B20 – Panel Surface Temperature

```
DS18B20 VCC  ──── 3.3V
DS18B20 GND  ──── GND
DS18B20 DATA ──── ESP32 GPIO 5
                       │
                    [4.7kΩ]
                       │
                     3.3V
```
> Mount DS18B20 flat against panel back with thermal adhesive tape.

## DHT22 – Ambient Temperature & Humidity

```
DHT22 VCC  ──── 3.3V
DHT22 GND  ──── GND
DHT22 DATA ──── ESP32 GPIO 4
                    │
                 [10kΩ]     (some modules include onboard pull-up)
                    │
                  3.3V
```

## BH1750 – Light Irradiance Sensor

```
BH1750 VCC  ──── 3.3V
BH1750 GND  ──── GND
BH1750 SDA  ──── ESP32 GPIO 21
BH1750 SCL  ──── ESP32 GPIO 22
BH1750 ADDR ──── GND  [I2C address 0x23]
```
> Mount face-up in a small diffuser dome on the enclosure top.

## GP2Y1010AU0F – Optical Dust / Soiling Sensor

```
                    ┌─────────┐
  3.3V ──[150Ω]────►│ V-LED(1)│
                    │         │
  ESP32 GPIO 16 ───►│ LED-GND(2)│  (active LOW — pull low to pulse LED)
                    │         │
  GP2Y AOUT ───────►│ AOUT(3) │──── ESP32 GPIO 34
                    │         │
  GND ─────────────►│ GND(4)  │
                    │         │
  3.3V ────────────►│ VCC(5)  │
                    └─────────┘
                         │
               [220µF cap between VCC and GND for noise filtering]
```

> Timing: Pulse LED LOW for 320µs, sample ADC at 280µs, then return HIGH.
> This is implemented in readDustPercent() in transmitter.ino.

## Power System (Solar Self-Powered)

```
Small Solar Panel (5–10W)
        │
        ▼
  TP4056 Module (USB/Solar input)
        │
        ▼
  18650 LiPo Cell (3.7V, 3000mAh)
        │
        ▼
  MT3608 Boost Converter (→ 5V)
        │
        ▼
  ESP32 VIN pin (5V in)
        │
  ESP32 3.3V out ──── Sensors
```

## I2C Address Map

| Device  | Address |
|---------|---------|
| INA226  | 0x40    |
| BH1750  | 0x23    |

No conflicts — all devices can share the same I2C bus (GPIO 21/22).
