# SCOUT
A modular RC rover built from cheap and easily available components and 3D printed parts, designed to use a phone for live‑streaming video while leveraging onboard cellular connectivity to transmit data and telemetry to a homebase


<img width="1920" height="1080" alt="rovah" src="https://github.com/user-attachments/assets/00bb53da-2263-4230-8190-7b21b68df76f" />


**GOAL**

1)Highly repairable and modular with only M3 screws for mounts, possess mounting holes throughout for future improvements (NO GLUES NO ZIP TIES)

2)Robust explorer, Be easily controlled from anywhere in the world and have good runtime

3)Be made from cheap and easily available parts

4)Use a Phone for its expensive compnents like SIM,Camera,GPS,Accelerometer,Gyroscope for free rather than purchasing standalone parts which are expensive

# SPEC SHEET

| Attribute | Specification |
|-----------|---------------|
| **Project Name** | SCOUT |
| **Version** | V1.0 |
| **Type** | 4WD Exploration Rover |
| **Chassis Material** | 3D Printed PLA (White) |
| **Chassis Dimensions** | 255mm × 200mm × 100mm |
| **Wheel Diameter** | 85mm |
| **Weight (Estimated)** | ~1.5-2.0 kg |
| **Payload Capacity** | ~2-3 kg |
| **Power Source** | 7.4V 6000mAh 2S LiPo |

---

## MECHANICAL SPECS

### Chassis

| Parameter | Value |
|-----------|-------|
| **Length** | 255 mm |
| **Width** | 200 mm |
| **Height** | 100 mm |
| **Material** | PLA (Polylactic Acid) |
| **Print Settings** | Layer Height: 0.2mm, Infill: 20-40% |
| **Color** | White |

<img width="1210" height="826" alt="Screenshot 2026-06-21 131621" src="https://github.com/user-attachments/assets/fe315a38-8868-4af9-9a89-1f47d28c5cb2" />


### Drive System

| Component | Specification |
|-----------|---------------|
| **Motors** | 4× 12V Geared DC Motors |
| **Motor MAX RPM** | 200 RPM @ 12V |
| **ESTIMATED RPM** | ~123 RPM @ 7.4V |
| **Motor Type** | Metal Geared (Yellow/Green) |
| **Wheel Diameter** | 85 mm |
| **Wheel Type** | Plastic Hub + Rubber Tire |
| **Drive Type** | 4WD (All-wheel drive) |
| **Steering Type** | Differential (Tank-style) |

<img width="652" height="557" alt="WhatsApp Image 2024-11-15 at 03 26 55" src="https://github.com/user-attachments/assets/784a21f7-7bfa-4869-b989-292e860ef0c4" />


simple and cheap dosent need to cross mach one just needs torque and keep the rover moving



##  ELECTRICAL SPECs

### Power System

| Component | Specification |
|-----------|---------------|
| **Battery** | 7.4V 6000mAh 2S LiPo (2S2P) |
| **Battery Chemistry** | Li-Ion (NMC 18650) |
| **Discharge Rate** | 3C |
| **Battery Connector** | XT60/DC Jack |
| **BMS** | Integrated |
| **Power Switch** | Rocker Switch (≥5A) |
| **Buck Converter** | LM2596 (7.4V → 5V, 3A) |
| **Charger** | 2S Li-ion Charger (8.4V 1A) |

### Power Distribution

```
Battery 7.4V
    │
    ├──► Power Switch ──► LM2596 ──► 5V Rail
    │                        │
    │                        ├── ESP32 (VIN)
    │                        ├── TB6612 VCC (Logic)
    │                        ├── INA219 VCC
    │                        ├── OLED VCC
    │                        ├── DHT22 VCC
    │                        └── 5V Fan (+)
    │
    └──► TB6612 VM (Motor Supply)
              │
              └── 4× DC Motors
```

### THEORATICAL Runtime 

| Condition | Current Draw | Runtime (6000mAh) |
|-----------|--------------|-------------------|
| **Idle** (WiFi, sensors) | ~300mA | **20 hours** |
| **Light Driving** | ~600mA | **10 hours** |
| **Normal Driving** | ~1000mA | **6 hours** |
| **Heavy Driving** | ~1800mA | **3.3 hours** |
| **Battery Saver** | ~150mA | **40 hours** |

---

<img width="1420" height="632" alt="ESP32-WROOM-32_1" src="https://github.com/user-attachments/assets/a12c48b9-3fb5-4977-be14-e911bb716506" />


### Pin Usage

| GPIO | Function | Notes |
|------|----------|-------|
| 25,26,27,14,12,13 | Left Motor Driver | TB6612 #1 |
| 32,2,23,19,5,18 | Right Motor Driver | TB6612 #2 |
| 33 | TB_STBY | Standby (shared) |
| 21 | I2C SDA | INA219 + OLED |
| 22 | I2C SCL | INA219 + OLED |
| 4 | DHT22 Data | Temperature/Humidity |
| 15 | Fan MOSFET | Cooling control |

### Sensors & Peripherals

| Component | Specification | I2C Address | Function |
|-----------|---------------|-------------|----------|
| **INA219** | 32V/2A Range | 0x40 | Voltage, Current, Power |
| **DHT22** | -40~80°C, 0-100% RH | N/A (1-Wire) | Temperature, Humidity |
| **SSD1306 OLED** | 128×64, I2C | 0x3C | Telemetry Display |

INA219

<img width="1057" height="793" alt="CJMCU-219 INA219" src="https://github.com/user-attachments/assets/9863ffd3-4f40-4976-9e31-9784e96f1456" />

DHT22

<img width="677" height="567" alt="DHT22" src="https://github.com/user-attachments/assets/7d32bd53-fe5e-4b8a-a22b-cc8716cfe4de" />

ssd1306 OLED

<img width="1920" height="1080" alt="SSD1306_OLED_Display(128x64)_Angle_View" src="https://github.com/user-attachments/assets/34a3ae56-7018-4212-8c5d-ba3bd72c6c20" />


## BOM
| Category       | Item                          | Qty | Notes/Specs        | Seller Link | Price |
|----------------|-------------------------------|-----|--------------------|-------------|-------|
| Main Controller | ESP32 DevKit V1              | 1   | DevKit V1          | [Robu.in](https://robu.in/product/esp-wroom-32-wifi-bluetooth-networking-smart-component-development-board/) | 409 |
| Power          | 7.4V 6000mAh 2S LiPo         | 1   | With BMS           | [Robu.in](https://robu.in/product/pro-range-nmc-18650-7-4v-6000mah-3c-2s2p-li-ion-battery-pack/) | 1199 |
| Power          | XT60 connector pair          | 1   | Male+Female        | [Amazon.in](https://www.amazon.in/dp/B0...) | 260 |
| Power          | Rocker switch ≥5A            | 1   | Toggle type        | [Amazon.in](https://www.amazon.in/Rocker-Switch-Toggle-Plastic-Electronics/dp/B0GWS1MJB6/) | 45 |
| Power          | LM2596 Buck Converter        | 1   | 7.4V→5V            | [Amazon.in](https://www.amazon.in/LM2596-DC-DC-Buck-Converter-Module/dp/B009P04YTO/) | 98 |
| Motor Drivers  | TB6612FNG                    | 2   | Dual driver        | Robu Services | 249 |
| Monitoring     | INA219                       | 1   | Current sensor     | [Amazon.in](https://www.amazon.in/dp/B0...) | 275 |
| Monitoring     | SSD1306 OLED 0.96"           | 1   | I2C OLED           | [Amazon.in](https://www.amazon.in/Robocraze-Semiconductor-Accessory-Display-Resolution/dp/B07Q7VZ716/) | 329 |
| Cooling        | 5V brushless fan (30–40mm)   | 1   | Exhaust fan        | [Amazon.in](https://www.amazon.in/Electronic-Spices-40x40x10mm-Brushless-Exhaust/dp/B0BC8FLML8/) | 180 |
| Cooling        | AO3400A MOSFET               | 1   | Logic-level        | [Amazon.in](https://www.amazon.in/dp/B09P1NZX65/) | 84 |
| Cooling        | 10kΩ resistor                | 1   | Gate pulldown      | [Amazon.in](https://www.amazon.in/10K-ohm-Watt-Resistor-Tolerance/dp/B09VH1K4GW/) | 74 |
| Capacitors     | 1000µF 16V electrolytic      | 1   | Main rail          | [Amazon.in](https://www.amazon.in/gp/product/B08X2PGPPV/) | 89 |
| Capacitors     | 100µF 16V electrolytic       | 2   | TB6612 rails       | [Amazon.in](https://www.amazon.in/dp/B0...) | 79 |
| Capacitors     | 0.1µF ceramic                | 2   | TB6612 rails       | [Amazon.in](https://www.amazon.in/0-1uF-104-Ceramic-Capacitor-Pack/dp/B0F5MYTYQ3/) | 79 |
| Hardware       | M3 Screw Kit                 | 1 set | Assorted sizes    | [Amazon.in](https://www.amazon.in/gp/product/B0FQWLQMXV/) | 360 |
| DRIVE          | 12V geared DC Motor          | 4   | 12V 200 RPM        | [Amazon.in](https://www.amazon.in/dp/B0...) | 500 |
| Cooling        | DHT22 temp+humidity sensor   | 1   | Fan controls       | [Amazon.in](https://www.amazon.in/AM2302-Temperature-Humidity-Sensor-Module/dp/B0CXMJH34D/) | 160 |
| Resistors      | 100Ω resistor                | 1   | Pull up            | [Amazon.in](https://www.amazon.in/dp/B0...) | 80 |
| Wires          | JST-XH connectors            | 4   | Battery connectors | [Amazon.in](https://www.amazon.in/Electronic-Spices-Connector-2-5mm-Female/dp/B0BKWDKB1W/) | 109 |
| Wires          | 18 AWG Silicone Wire (1m)    | 1   | 20A Tinned Copper  | [Amazon.in](https://www.amazon.in/gp/product/B09VLDN5LN/) | 300 |
| DRIVE          | 85 mm Wheels                 | 4   | Wheels             | [Amazon.in](https://www.amazon.in/INVENTO-Plastic-Robotic-Durable-Rubber/dp/B07YNQSLKX/) | 749 |
| Hardware       | M3 standoffs                 | 20  | Standoffs          | [Amazon.in](https://www.amazon.in/gp/product/B07T34Q6G6/) | 300 |
| Hardware       | 12×8 Perfboard               | 1   | Perfboard          | [Amazon.in](https://www.amazon.in/xcluma-Double-Copper-Prototype-Universal/dp/B073DF2CB9/) | 125 |
| Phone Mount    | Rubber Bands                 | 3   | Holds phone        | [Amazon.in](https://www.amazon.in/cart/smart-wagon?newItems=2356efb0-3c12-4c21-a734-ce8c3620b643,1) | 150 |
| Charger        | 2S Li-ion battery charger    | 1   | For battery        | [Robu.in](https://robu.in/product/lithium-battery-charger-8-4v-1a-with-dc-plug-2-indicators/) | 676 |
| Chassis        | White PLA 1kg                | 1   | Printed for chassis| [Amazon.in](https://www.amazon.in/WOL-3D-Filament-Improved-Formula/dp/B08D6N1HHK/) | 899 |
TOTAL PRICE-₹7875(85$)

85 dollars may seem like a lot which it is but lot of these components are easily found in old electronics like capacitors,resistors,M3 screws,Perfboards,Brushless DC fans,Mosfets and etc.
Just from me salvaging old electronics im able to replicate this project in around 40$

<img width="1282" height="828" alt="Schematic_Rover-Scheme1_2026-06-05" src="https://github.com/user-attachments/assets/0d0c123a-ad87-43aa-9124-d182ff4040d1" />



| Feature | Details |
|---------|---------|
| **WiFi AP Mode** | Direct phone/PC control (30-100m) |
| **Bluetooth P2P** | Direct control (10-50m) |
| **WiFi STA Mode** | Home network control (router dependent) |
| **4G Cloud Mode** | Global control via relay server |
| **WebSocket** | Real-time telemetry (100Hz control, 5Hz telemetry) |
| **HTTP Server** | Web dashboard (192.168.4.1) |

---

##  CONTROL INTERFACES

| Interface | Access Method | Features |
|-----------|---------------|----------|
| **Web Dashboard** | Browser (http://192.168.4.1) | Joystick, Telemetry, Mode Switching |
| **Bluetooth Serial** | Bluetooth Terminal App | Command-based control (F, B, L, R, S) |
| **PC Dashboard** | PC Browser (4G Mode) | Full control + video overlay |

---

##  SAFETY FEATURES

| Feature | Description |
|---------|-------------|
| **Watchdog Timer** | Stops motors if no command received for 500ms,Preventing it from endlessly driving if connection is cut|
| **Battery Saver** | Reduces CPU, disables peripherals at low battery |
| **Thermal Protection** | Fan activates at 40°C, turns off at 35°C |
| **Overcurrent Protection** | Software limit (INA219 monitoring) |
| **Emergency Stop** | Hardware + Software E-Stop button |

---


---

**Why did i make this project?**
