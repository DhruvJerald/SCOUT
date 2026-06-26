<div align="center">

# SCOUT
<img width="666" height="375" alt="rover-removebg-preview" src="https://github.com/user-attachments/assets/4bab35ab-d2d3-4475-a19b-1955650a718c" />


**A modular 4WD exploration rover built from cheap, repairable parts**

*Uses a phone for camera, GPS, cellular, and IMU — no expensive standalone modules*

---

![rover render](https://github.com/user-attachments/assets/00bb53da-2263-4230-8190-7b21b68df76f)

</div>

---

## What it is

SCOUT is a remote-controlled ground rover designed to be driven from anywhere in the world. The phone mounted on the roof does the heavy lifting — streaming video, providing GPS, accelerometer, gyroscope, and cellular data — while the ESP32 handles motors, sensors, and real-time control.

The whole chassis is 3D printed in PLA and held together exclusively with M3 screws. No glue. No zip ties. Every part is replaceable.

---

## Goals

- **Repairable first** — M3 screws throughout, mounting holes everywhere, modular boards
- **Phone as compute** — SIM, camera, GPS, IMU all from a phone you already own
- **Cheap and sourceable** — every component available on Amazon.in or Robu.in
- **Controllable from anywhere** — WiFi AP, Bluetooth, or 4G cloud relay

---

## Specs

| | |
|---|---|
| **Chassis** | 3D printed PLA — 255 × 210 × 114 mm |
| **Weight** | ~1.5 – 2.0 kg |
| **Payload** | ~2 – 3 kg |
| **Motors** | 4× 12V 200 RPM geared DC (metal gear) |
| **Estimated speed** | ~123 RPM at 7.4V battery |
| **Steering** | Differential (tank style, turns in place) |
| **Wheels** | 85 mm rubber |
| **Battery** | 7.4V 6000mAh 2S LiPo with BMS |

---

## Architecture

<img width="1190" height="894" alt="1" src="https://github.com/user-attachments/assets/d518493c-3983-4eab-9a86-a07f1d825c37" />


### Power Distribution

```
Battery 7.4V
    │
    ├── Power Switch ── LM2596 ── 5V Rail ── ESP32
    │                                    ── TB6612 logic
    │                                    ── INA219
    │                                    ── OLED
    │                                    ── DHT22
    │                                    ── Fan
    │
    └── TB6612 VM (motor rail) ── 4× DC Motors
```

---

## Control Modes

| Mode | Transport | Range | Latency |
|------|-----------|-------|---------|
| WiFi AP | Direct to ESP32 | ~50m open air | 20 – 50ms |
| Bluetooth | BLE serial | ~10m | 10 – 30ms |
| 4G Cloud | Internet relay via VPS | Unlimited | 100 – 300ms |

The phone bridges all three. It picks the fastest available path automatically.

---

## Hardware

### Electronics

| Component | Purpose | Qty |
|-----------|---------|-----|
| ESP32 DevKit V1 | Main controller | 1 |
| TB6612FNG | Dual motor driver | 2 |
| INA219 | Voltage + current monitor | 1 |
| DHT22 | Temperature + humidity | 1 |
| SSD1306 OLED 0.96" | Status display | 1 |
| LM2596 buck converter | 7.4V → 5V | 1 |
| AO3400A MOSFET | Fan control | 1 |
| 5V brushless fan 40mm | Cooling | 1 |

### Drive

| Component | Qty |
|-----------|-----|
| 12V 200RPM geared DC motor (metal gear) | 4 |
| 85mm rubber wheels | 4 |
| 7.4V 6000mAh 2S LiPo + BMS | 1 |
| XT60 connector pair | 1 |

### Chassis

| Component | Qty |
|-----------|-----|
| White PLA 1kg | 1 |
| M3 screw kit (assorted) | 1 set |
| M3 standoffs | 20 |
| 12×8 perfboard | 1 |

> All mechanical connections use M3 screws only. The chassis has mounting holes throughout for future modules.

---

## Schematic

![schematic](https://github.com/user-attachments/assets/0d0c123a-ad87-43aa-9124-d182ff4040d1)

---

## BOM (India)

| Item | Price (INR) |
|------|------------|
| ESP32 DevKit V1 | 409 |
| 7.4V 6000mAh 2S LiPo | 1199 |
| XT60 connector pair | 260 |
| Rocker switch | 45 |
| LM2596 buck converter | 98 |
| TB6612FNG × 2 | 249 |
| INA219 | 275 |
| SSD1306 OLED | 329 |
| 5V brushless fan | 180 |
| AO3400A MOSFET | 84 |
| Resistors (10kΩ + 100Ω) | 154 |
| Capacitors (1000µF + 100µF × 2 + 0.1µF × 2) | 247 |
| M3 screw kit | 360 |
| 12V 200RPM motors × 4 | 500 |
| DHT22 | 160 |
| JST-XH connectors | 109 |
| 18AWG silicone wire | 300 |
| 85mm wheels × 4 | 749 |
| M3 standoffs | 300 |
| Perfboard | 125 |
| Phone mount (rubber bands) | 150 |
| 2S LiPo charger | 676 |
| White PLA 1kg | 899 |
| **Total** | **~₹7,875** |

> Many of these — capacitors, resistors, perfboard, fan, MOSFET — can be salvaged from old electronics.

---

## Features

**Safety**
- Motor watchdog — stops motors after 500ms with no signal
- Emergency stop — software command and hardware switch
- Battery saver mode — shuts down WiFi, OLED, and fan when voltage drops

**Monitoring**
- Live voltage, current, and power via INA219
- Ambient temperature and humidity via DHT22
- Status display on SSD1306 OLED
- All telemetry streamed to phone dashboard

**Expansion**

The chassis has mounting holes and an I2C header for adding modules without redesigning anything.

| Category | Possible additions |
|----------|--------------------|
| Navigation | GPS module, IMU, ultrasonic, LiDAR |
| Vision | ESP32-CAM for wider FOV |
| Actuation | Servo arm, LED strip |
| Environment | Air quality, CO2 sensor |

Software upgrades possible once hardware is stable: waypoint navigation, obstacle avoidance, ROS2 integration, data logging to phone storage.

---

## Images

<!-- Add your own photos here -->

![chassis render](https://github.com/user-attachments/assets/c412a633-3a0c-42e1-b531-9bf9a19d7bf3)

![second render](https://github.com/user-attachments/assets/7b8ea1f7-f10d-40bc-89fd-80195fb45253)

---

## Build and flash

Requirements: Arduino IDE 2.x, ESP32 board support, the following libraries:

```
NimBLE-Arduino        h2zero
ESPAsyncWebServer     lacamera
AsyncTCP              dvarrel
Adafruit SSD1306      Adafruit
Adafruit GFX Library  Adafruit
Adafruit INA219       Adafruit
DHT sensor library    Adafruit
Adafruit Unified Sensor  Adafruit
ArduinoJson           Benoit Blanchon
```

Flash `esp32/scout_firmware.ino`. Open Serial Monitor at 115200 baud and verify all `[BOOT]` lines appear before connecting hardware.

Full wiring, pin assignments, fault diagnosis, and step-by-step test sequence are in [`SETUP.md`](./SETUP_V3.md).

---

## Status

<!-- Update this as you build -->

- [ ] Chassis printed
- [ ] Electronics wired and tested
- [ ] Firmware flashed and verified
- [ ] WiFi control working
- [ ] BLE control working
- [ ] Phone dashboard working
- [ ] 4G relay working
- [ ] First outdoor run

---

*Personal project. Parts sourced from Amazon.in and Robu.in. Chassis designed in [your CAD tool].*
