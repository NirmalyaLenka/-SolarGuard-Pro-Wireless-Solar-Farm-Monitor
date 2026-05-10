# Receiver (Display Unit) — Full Wiring Schematic

## Block Diagram

```
                        ┌─────────────────────────────────────────┐
                        │           ESP32 DevKit V1                │
                        │                                          │
  OLED SDA ───────────►│ GPIO 21 (SDA / I2C)                      │
  OLED SCL ───────────►│ GPIO 22 (SCL / I2C)                      │
                        │                                          │
  BTN_PREV ───────────►│ GPIO 25 (INPUT_PULLUP)                   │
  BTN_NEXT ───────────►│ GPIO 26 (INPUT_PULLUP)                   │
  BTN_SET  ───────────►│ GPIO 27 (INPUT_PULLUP)                   │
                        │                                          │
         5V  ───────────│ VIN                                      │
         GND ───────────│ GND                                      │
                        └─────────────────────────────────────────┘
```

## SSD1306 / SH1106 OLED (128×64, I2C)

```
OLED VCC  ──── 3.3V    (some modules 5V tolerant — check yours)
OLED GND  ──── GND
OLED SDA  ──── ESP32 GPIO 21
OLED SCL  ──── ESP32 GPIO 22
```

Default I2C address: **0x3C** (solder bridge for 0x3D if needed)

## Buttons (3× momentary pushbutton)

```
BTN_PREV:
  One terminal ──── ESP32 GPIO 25
  Other terminal ── GND
  (No external resistor needed — uses INPUT_PULLUP)

BTN_NEXT:
  One terminal ──── ESP32 GPIO 26
  Other terminal ── GND

BTN_SET:
  One terminal ──── ESP32 GPIO 27
  Other terminal ── GND
```

## Button Functions Summary

| Button | Normal mode | Set mode (settings pages) |
|--------|-------------|--------------------------|
| PREV   | Previous page ◄ | Decrease value ▼ |
| NEXT   | Next page ► | Increase value ▲ |
| SET    | Enter set mode | Confirm & save |

> Long-press SET (hold 2s) can be added for factory reset — see TROUBLESHOOTING.md

## Enclosure Recommendation

```
┌──────────────────────────────┐
│   [ OLED 1.3" Display ]      │  ← Cutout for display
│                              │
│  [PREV]   [NEXT]   [SET]     │  ← 3 buttons with caps
│                              │
│  Power LED ●                 │  ← Optional status LED
└──────────────────────────────┘
Wall-mount ABS box, ~100×60×30mm
```
