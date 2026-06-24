# MSRP-1 Rover V3 — Setup Guide

## What's new in V3 vs V2

| Feature | V2 | V3 |
|---------|----|----|
| BLE transport | ✗ | ✓ NimBLE UART server |
| WiFi AP | ✓ | ✓ unchanged |
| 4G remote control | Optional | ✓ via phone bridge |
| Battery saver mode | ✗ | ✓ BLE-only, WiFi/OLED/Fan off |
| Auto transport display | ✗ | ✓ OLED shows BLE/WiFi/none |
| Transport switching | ✗ | ✓ phone picks BLE or WiFi automatically |
| Phone acts as bridge | ✗ | ✓ bridges VPS ↔ ESP32 |

---

## File Structure

```
rover-v3/
├── esp32/
│   └── rover_v3_firmware.ino   ← Flash to ESP32
├── phone/
│   └── index.html              ← Open on phone browser (Chrome Android)
├── server/
│   ├── cloud_relay.js          ← Run on VPS for remote control
│   └── package.json
└── SETUP_V3.md                 ← This file
```

---

## How the transport chain works

```
PC (home)
   │
   │  WebSocket (internet)
   ▼
VPS cloud relay          ← only needed for remote 4G control
   │
   │  WebSocket (4G)
   ▼
Phone (mounted on rover)
   │
   ├── BLE UART (primary, <10m, lowest power)
   │      or
   └── WiFi WebSocket (secondary, <50m, faster)
          │
          ▼
       ESP32 (rover)
```

**The phone is the bridge.** It holds two connections at once:
- Outward to the VPS (or directly to PC) via 4G or WiFi
- Inward to the ESP32 via BLE or WiFi AP

**Transport priority on the phone:**
1. BLE — if available (Chrome Android only) and ESP32 in range
2. WiFi WebSocket — if phone is connected to ROVER-V3 AP
3. 4G relay — if neither of the above works

---

## Step 1 — Flash ESP32 firmware

### 1.1 New library to install (all others same as V2)

In Arduino IDE → Tools → Manage Libraries, search and install:

| Library | Author | Notes |
|---------|--------|-------|
| **NimBLE-Arduino** | h2zero | NEW — replaces BlueDroid BLE stack |

All other libraries from V2 remain the same (ESPAsyncWebServer, AsyncTCP, Adafruit SSD1306, INA219, DallasTemperature, OneWire, ArduinoJson).

### 1.2 Board settings (unchanged from V2)
- Board: ESP32 Dev Module
- Upload Speed: 921600
- CPU Frequency: 240 MHz
- Partition Scheme: Default 4MB with spiffs
- Port: your COM port

### 1.3 Flash and verify boot output

```
[BOOT] MSRP-1 V3 starting...
[BOOT] Motor drivers enabled
[BOOT] OLED OK
[BOOT] INA219 OK
[BOOT] DS18B20 OK (1 sensor)
[BOOT] BLE advertising as 'MSRP1-Rover'
[BOOT] WiFi AP: SSID='ROVER-V3'  IP=192.168.4.1
[BOOT] HTTP + WebSocket server on port 80
[BOOT] All tasks started. Rover ready.
[BOOT] Transports: BLE (primary) + WiFi AP (secondary)
```

---

## Step 2 — Phone setup (Chrome Android recommended)

1. Open Chrome on Android
2. Go to `file:///` and open `phone/index.html`, **OR** host it locally:
   - Connect phone to your PC via USB
   - Run `python3 -m http.server 8080` in the rover-v3/phone folder
   - Open `http://YOUR_PC_IP:8080` on the phone
3. The dashboard opens. It will:
   - Attempt BLE connection to `MSRP1-Rover` automatically (Chrome Android only)
   - Fall back to WiFi WebSocket if BLE is unavailable or denied

### iOS note
Apple blocks the Web Bluetooth API in Safari and Chrome iOS. On iPhone, the phone will automatically use the WiFi path only. Connect the phone to the **ROVER-V3** WiFi network first.

### BLE permission prompt
Chrome on Android will show a Bluetooth device picker the first time. Select **MSRP1-Rover** from the list. After pairing, subsequent connections are automatic.

---

## Step 3 — Transport modes explained

### Mode 1: BLE Direct (phone ↔ ESP32)
- Phone within ~10m of rover
- Lowest power: ESP32 draws ~30–50mA total in BLE-only mode
- Latency: 20–80ms
- No WiFi needed
- OLED shows `Link: BLE`

### Mode 2: WiFi Direct (phone ↔ ESP32 AP)
- Phone connects to ROVER-V3 WiFi network
- Range ~50m open air
- Latency: 5–20ms (fastest)
- ESP32 draws ~200–300mA
- OLED shows `Link: WiFi`

### Mode 3: 4G Relay (PC → VPS → phone → ESP32)
- Phone bridges VPS WebSocket to ESP32 (BLE or WiFi)
- Works from anywhere in the world
- Latency: 50–250ms depending on VPS location
- Requires VPS running cloud_relay.js
- OLED still shows `Link: BLE` or `Link: WiFi` (last mile to ESP32)

---

## Step 4 — Battery saver mode

### Manual trigger (from dashboard)
Click **🔋 Battery Saver** button. The ESP32:
- Turns WiFi off (saves ~150–200mA)
- Turns OLED off (saves ~20mA)
- Turns fan off
- Stays on BLE only

To wake back up: click **⚡ Normal** in the phone dashboard (sent over BLE).

### Auto trigger
If battery voltage drops below **7.0V** (~20% for 2S Li-ion), battery saver activates automatically. The serial monitor will print:
```
[PWR] Low battery (6.94V) → auto battery saver
```

### Power consumption comparison

| Mode | Approx. current draw |
|------|----------------------|
| Normal (WiFi + BLE + OLED + idle motors) | 220–300mA |
| Normal (motors running) | 500–1500mA |
| Battery saver (BLE only, idle) | 30–50mA |
| Battery saver (motors running) | 300–1200mA |

---

## Step 5 — VPS relay (for remote 4G control)

Only needed if controlling from home while rover is elsewhere.

### 5.1 Create a VPS
DigitalOcean, Linode, or AWS Lightsail. A $4–5/month 1GB RAM Ubuntu server is enough.

### 5.2 Install Node.js on VPS
```bash
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install nodejs
```

### 5.3 Upload and run
```bash
scp -r rover-v3/server/ user@YOUR_VPS_IP:~/msrp1/
ssh user@YOUR_VPS_IP
cd ~/msrp1
npm install
PUBLIC_IP=YOUR_VPS_IP node cloud_relay.js
```

### 5.4 Open firewall port
```bash
sudo ufw allow 8081/tcp
```

### 5.5 Update phone dashboard
In `phone/index.html`, find and update:
```javascript
relay:  'wss://YOUR_VPS_IP:8081/?role=phone'
```

In `pc_dashboard` (or the relay mode of phone dashboard), update:
```javascript
'wss://YOUR_VPS_IP:8081/?role=pc'
```

---

## Pin reference (unchanged from V2)

| GPIO | Function | Hardware note |
|------|----------|---------------|
| 2 | R Front AIN2 | — |
| 4 | DS18B20 | 4.7kΩ pull-up to 3.3V |
| 5 | R Rear BIN2 | — |
| 12 | L Rear BIN2 | — |
| 13 | L Rear PWMB | LEDC ch1 |
| 14 | L Rear BIN1 | — |
| 15 | Fan gate | 100Ω series + 10kΩ pull-down to GND |
| 16 | RESERVED RX2 | Future GPS UART |
| 17 | RESERVED TX2 | Future GPS UART |
| 18 | R Rear PWMB | LEDC ch3 |
| 19 | R Rear BIN1 | — |
| 21 | I2C SDA | 4.7kΩ pull-up to 3.3V |
| 22 | I2C SCL | 4.7kΩ pull-up to 3.3V |
| 23 | R Front PWMA | LEDC ch2 |
| 25 | L Front AIN1 | — |
| 26 | L Front AIN2 | — |
| 27 | L Front PWMA | LEDC ch0 |
| 32 | R Front AIN1 | — |
| 33 | TB6612 STBY | Shared both drivers |
| 34, 35 | Expansion inputs | INPUT ONLY — cannot drive output |

**Never use:** GPIO 0, 1, 3, 6–11

---

## WebSocket / BLE command protocol (unchanged)

```json
{ "type": "drive", "left": 180, "right": 180 }
{ "type": "stop" }
{ "type": "power", "mode": "saver" }
{ "type": "power", "mode": "normal" }
{ "type": "ping" }
```

Telemetry from rover (every 100ms):
```json
{
  "type": "telem",
  "v": 7.82, "mA": 340, "mW": 2659, "temp": 31.5,
  "fan": false, "wifiCl": 1, "ble": true,
  "transport": "ble",
  "power": "normal"
}
```

The `transport` field shows which link delivered the last command (`"ble"`, `"wifi"`, or `"none"`).

---

## Build and test order (additions to V2 order)

Run all V2 steps 1–9 first. Then:

10. With rover running, open Chrome Android, open phone/index.html, tap **Connect BLE**, select MSRP1-Rover. Verify telemetry appears and joystick drives motors.
11. Test battery saver: tap 🔋 button. Verify WiFi AP disappears from your PC's network list. Verify OLED goes dark. Verify BLE stays connected.
12. Tap ⚡ Normal. Verify WiFi AP reappears, OLED turns back on.
13. (Optional) Deploy VPS, update URLs in phone/index.html, test relay mode from a different network.
