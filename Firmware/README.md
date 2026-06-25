***🤖🤖🤖🤖🤖 ALL THESE FILES ARE AI GENERATED AND ARENT TESTED AND PURELY FOR DEPICTING THE EXECUTION OF THIS PROJECT NOT TO BE USED IN THE ACTUAL PROJECT !! ***


THIS FIRMWARE ISNT TESTED YET IT WILL MOST LIKELY FAIL 

# SCOUT SETUP FILE

---

## Contents

1. System Architecture
2. Hardware Architecture
3. Power Architecture
4. Pin Assignment and Wiring
5. Transport Layer Architecture
6. Software Architecture
7. Firmware Setup (ESP32)
8. Phone Dashboard Setup
9. VPS Relay Setup
10. Battery Saver Mode
11. Communication Protocol Reference
12. Build and Test Sequence
13. Fault Diagnosis


---

## 1. System Architecture

The system has three physical nodes. Every command and every telemetry reading
travels through all three in sequence.

```
+------------------+        internet         +------------------+
|    PC (home)     |  <--------------------> |   VPS (cloud)    |
|                  |     WebSocket / HTTPS   |   cloud_relay.js |
|  Browser opens   |                         |   public IP      |
|  phone/index.html|                         |  Node.js server  |
+------------------+                         +--------+---------+
                                                      |
                                                   4G / WiFi
                                                      |
                                             +--------v---------+
                                             |  Phone (on rover)|
                                             |  Chrome browser  |
                                             |  index.html      |
                                             |                  |
                                             |  Holds TWO       |
                                             |  connections:    |
                                             |  1. outward VPS  |
                                             |  2. inward ESP32 |
                                             +--------+---------+
                                                      |
                                          BLE or WiFi (last mile)
                                                      |
                                             +--------v---------+
                                             |  ESP32 (rover)   |
                                             |  BLE UART server |
                                             |  WiFi AP server  |
                                             |  WebSocket server|
                                             +--------+---------+
                                                      |
                                              GPIO pins (wires)
                                                      |
                              +-----------+-----------+-----------+
                              |           |           |           |
                        +-----v----+ +----v----+ +----v---+ +----v----+
                        | TB6612 1 | |TB6612 2 | | INA219 | |DS18B20 |
                        | L motors | |R motors | | power  | | temp   |
                        +-----+----+ +----+----+ +--------+ +--------+
                              |           |
                       +------+    +------+
                       |           |
                  +----v----+ +----v----+
                  |LF motor | |RF motor |
                  |LR motor | |RR motor |
                  +---------+ +---------+
```

The phone is the bridge. It is physically mounted on the rover and runs the
browser app. It maintains one connection outward (to VPS or PC) and one
connection inward (to ESP32 via BLE or WiFi). When you push the joystick on
your PC, the command travels: PC -> VPS -> phone -> ESP32 -> motors.

When the phone and rover are close to your PC on the same network, the VPS
is bypassed entirely. When the phone is within 10m of the ESP32 and BLE is
available, WiFi is bypassed as well. The system always takes the shortest path.

---

## 2. Hardware Architecture

```
                    ROVER PHYSICAL LAYOUT (top view)
    +-------------------------------------------------------+
    |  [OLED display]                    [Expansion header] |
    |                                                        |
    |   +---------------+     +---------------+             |
    |   |   TB6612 #1   |     |   TB6612 #2   |             |
    |   |  (left side)  |     |  (right side) |             |
    |   +---+-----------+     +----------+----+             |
    |       |                            |                   |
    |  LF--LR                        RF--RR                 |
    |  motors                        motors                  |
    |                                                        |
    |   +---------------------------+                        |
    |   |       ESP32 DevKit V1     |   [INA219]             |
    |   |                           |   [DS18B20]            |
    |   +---------------------------+                        |
    |                                                        |
    |   [Fan + AO3400 MOSFET]          [Power switch]        |
    |                                                        |
    |   +---------------------------+                        |
    |   |  2x 18650 cells + 2S BMS |                        |
    |   +---------------------------+                        |
    +-------------------------------------------------------+
    [LF wheel]  [LR wheel]           [RF wheel]  [RR wheel]


    LAYER STACK (electronics, bottom to top)

    Bottom plate:  4 motors, wheels, motor mounts
    Middle plate:  18650 cells, BMS, power switch, INA219
    Top plate:     ESP32, TB6612 x2, DS18B20, fan
    Roof rail:     OLED, expansion header, phone mount
```

---

## 3. Power Architecture

The INA219 sits in series on the positive rail so it measures total current
drawn by the entire system. The LM2596 buck converter steps 7.4V down to 5V
for the ESP32 and logic. The TB6612 drivers receive the raw battery voltage
directly as their motor supply.

```
  2x 18650 cells (7.4V nominal, 8.4V full, 6.0V cutoff)
         |
         |  positive rail
         v
     [2S BMS]  <-- cuts off if cell < 3.0V or current > rated limit
         |
         v
    [1N5408 diode]  <-- reverse polarity protection, 0.6V drop
         |
         v
     [3A fuse]  <-- overcurrent protection (motor short circuit)
         |
         v
  [Power switch]
         |
         +---------------------------+
         |                           |
         v                           v
     [INA219]                  [LM2596 buck]
    VIN+ to VIN-               7.4V --> 5V
    measures all                    |
    system current                  +----------+----------+
         |                          |          |          |
         v                          v          v          v
    [TB6612 VM]               [ESP32 VIN]  [OLED VCC]  [Fan 5V]
    raw 7.4V for                3.3V reg    (3.3V)     (5V)
    motors                      onboard
         |
    +----+----+
    |         |
    v         v
 [TB6612 #1] [TB6612 #2]
 LF + LR     RF + RR
 motors      motors


  VOLTAGE AT EACH POINT (measure with multimeter to verify)

  Point                    Expected range      If wrong
  -------                  --------------      --------
  Battery terminals        6.0 - 8.4V          BMS tripped or dead cells
  After diode              0.6V less than bat   Diode missing or reversed
  After fuse               same as post-diode   Fuse blown
  INA219 VIN+ to VIN-      < 0.1V (shunt drop) INA219 bypassed (wrong wire)
  TB6612 VM                same as post-fuse    Break in motor supply wire
  LM2596 output            4.8 - 5.2V          Buck not adjusted or failed
  ESP32 VIN                4.8 - 5.2V          Wire break to ESP32
  ESP32 3.3V pin           3.1 - 3.4V          ESP32 internal reg failed
```

### Current budget

```
  Component                Idle      Active (peak)
  ---------                ----      -------------
  ESP32 (WiFi active)      150mA     350mA
  ESP32 (BLE only)          30mA      80mA
  OLED SSD1306              20mA      20mA
  INA219                     1mA       1mA
  DS18B20                    1mA       1mA
  Fan (5V, small)             0mA     200mA
  4x TT motors (idle)        20mA      20mA
  4x TT motors (running)       --     800mA
  4x TT motors (stall)         --    2000mA
  -------------------------------------------------------
  Normal mode total          200mA   ~2600mA peak
  Battery saver total         50mA   ~2100mA (motors only)

  Runtime estimate (5000mAh 2S LiPo):
    Driving normally:  ~1.5 - 2.5 hours
    Battery saver idle:  ~50 hours (BLE connected, stationary)
```

---

## 4. Pin Assignment and Wiring

### ESP32 DevKit V1 pinout map

Only the pins used in this project are labeled. Blank pins are free for
expansion unless marked NEVER USE.

```
                    ESP32 DevKit V1 (38-pin)
                    +=========================+
               3.3V |  1                  38 | GND
              RESET |  2                  37 | GPIO23  --> R_F_PWMA  (LEDC ch2)
    NEVER  --> GND  |  3                  36 | GPIO22  --> I2C SCL
    USE         5V  |  4                  35 | GPIO21  --> I2C SDA
                    |  5                  34 | GPIO19  --> R_R_BIN1
   INPUT      GPIO34|  6                  33 | GPIO18  --> R_R_PWMB  (LEDC ch3)
   ONLY       GPIO35|  7                  32 | GPIO5   --> R_R_BIN2
   (no        GPIO32|  8 --> R_F_AIN1    31 | GPIO17  RESERVED TX2
   pullup)    GPIO33|  9 --> TB6612 STBY 30 | GPIO16  RESERVED RX2
              GPIO25| 10 --> L_F_AIN1    29 | GPIO4   --> DS18B20 data
              GPIO26| 11 --> L_F_AIN2    28 | GPIO2   --> R_F_AIN2
              GPIO27| 12 --> L_F_PWMA    27 | GPIO15  --> Fan gate
              GPIO14| 13 --> L_R_BIN1    26 | GPIO13  --> L_R_PWMB  (LEDC ch1)
              GPIO12| 14 --> L_R_BIN2    25 | GPIO12  already used (left)
    NEVER  --> GND  | 15                  24 | GND
    USE    --> 3.3V | 16                  23 | 5V (from LM2596)
    GPIO6-11 FLASH  |  (internal, never break out)
                    +=========================+

    PINS TO NEVER USE:
    GPIO 0   -- Boot button. Pulling LOW during boot enters flash mode.
    GPIO 1   -- UART TX (USB serial debug output)
    GPIO 3   -- UART RX (USB serial debug input)
    GPIO 6-11-- Connected to internal flash memory. Using these destroys flash.
    GPIO34-39-- INPUT ONLY. No internal pull-up. Cannot drive anything.

    RESERVED (do not assign):
    GPIO16   -- Future UART2 RX (GPS module)
    GPIO17   -- Future UART2 TX (GPS module)
```

### Full wiring table

```
  ESP32 GPIO   Direction   Connects to           Notes
  ----------   ---------   -----------           -----
  GPIO2        OUT         TB6612#2 AIN2         R Front motor dir B
  GPIO4        IN          DS18B20 DATA          4.7k pullup to 3.3V required
  GPIO5        OUT         TB6612#2 BIN2         R Rear motor dir B
  GPIO12       OUT         TB6612#1 BIN2         L Rear motor dir B
  GPIO13       OUT(PWM)    TB6612#1 PWMB         L Rear speed, LEDC ch1
  GPIO14       OUT         TB6612#1 BIN1         L Rear motor dir A
  GPIO15       OUT         AO3400 gate           Fan MOSFET, 100R series + 10k pulldown
  GPIO18       OUT(PWM)    TB6612#2 PWMB         R Rear speed, LEDC ch3
  GPIO19       OUT         TB6612#2 BIN1         R Rear motor dir A
  GPIO21       IN/OUT      SDA bus               INA219 + OLED shared, 4.7k pullup
  GPIO22       IN/OUT      SCL bus               INA219 + OLED shared, 4.7k pullup
  GPIO23       OUT(PWM)    TB6612#2 PWMA         R Front speed, LEDC ch2
  GPIO25       OUT         TB6612#1 AIN1         L Front motor dir A
  GPIO26       OUT         TB6612#1 AIN2         L Front motor dir B
  GPIO27       OUT(PWM)    TB6612#1 PWMA         L Front speed, LEDC ch0
  GPIO32       OUT         TB6612#2 AIN1         R Front motor dir A
  GPIO33       OUT         TB6612#1 STBY         Shared STBY for both TB6612s
```

### TB6612 motor driver wiring (per chip)

Each TB6612 has two independent H-bridge channels (A and B), each driving
one motor. The chip needs 3 pins per motor (direction A, direction B, PWM)
plus one shared STBY pin.

```
  TB6612 #1 (left side)          TB6612 #2 (right side)
  +-----------------+            +-----------------+
  | VM   --> bat+   |            | VM   --> bat+   |
  | GND  --> GND    |            | GND  --> GND    |
  | VCC  --> 3.3V   |            | VCC  --> 3.3V   |
  | STBY --> GPIO33 |            | STBY --> GPIO33 |  (shared)
  |                 |            |                 |
  | AIN1 --> GPIO25 |            | AIN1 --> GPIO32 |
  | AIN2 --> GPIO26 |            | AIN2 --> GPIO2  |
  | PWMA --> GPIO27 |            | PWMA --> GPIO23 |
  | AO1  --> LF+    |            | AO1  --> RF+    |
  | AO2  --> LF-    |            | AO2  --> RF-    |
  |                 |            |                 |
  | BIN1 --> GPIO14 |            | BIN1 --> GPIO19 |
  | BIN2 --> GPIO12 |            | BIN2 --> GPIO5  |
  | PWMB --> GPIO13 |            | PWMB --> GPIO18 |
  | BO1  --> LR+    |            | BO1  --> RR+    |
  | BO2  --> LR-    |            | BO2  --> RR-    |
  +-----------------+            +-----------------+

  Motor direction truth table (same for all four motors):
  IN1    IN2    PWM    Result
  HIGH   LOW    0-255  Forward (speed proportional to PWM)
  LOW    HIGH   0-255  Reverse
  HIGH   HIGH   any    Active brake (motor shorted, decelerates fast)
  LOW    LOW    any    Coast (motor free-spinning)
```

### Fan MOSFET wiring

```
  5V rail ----+
              |
           [Fan +]
           [Fan -]
              |
           [Drain]
           AO3400
           [Gate] <---- [100R resistor] <---- GPIO15
           [Source]
              |
           [GND] <---- [10k resistor] <---- also [Gate]
                        (pull-down)

  The 10k from gate to GND ensures GPIO15 is LOW at boot.
  GPIO15 has an internal pull-up that could turn the fan on during boot.
  The external 10k overrides it. Without this resistor, fan may
  briefly switch on every time the ESP32 resets.
```

### I2C bus wiring

Both INA219 and SSD1306 OLED share the same two wires. Pull-up resistors
are needed once on the bus, not once per device.

```
  ESP32 3.3V ----+----+
                 |    |
               4.7k  4.7k   (one pair for the entire I2C bus)
                 |    |
  GPIO21 (SDA) --+--+-+--+------ INA219 SDA
                    |            SSD1306 SDA
  GPIO22 (SCL) -----+--+-------- INA219 SCL
                       |         SSD1306 SCL

  I2C addresses:
    INA219:   0x40 (default, pins A0 A1 both GND)
    SSD1306:  0x3C (most common) or 0x3D (some modules)
    Firmware tries 0x3C first, falls back to 0x3D automatically.

  To scan for addresses: upload i2c_scanner.ino (Arduino IDE examples)
  before wiring sensors. Any device found at an unexpected address
  means a different module variant.
```

---

## 5. Transport Layer Architecture

The system supports three transport paths. They are not exclusive: BLE and
WiFi run simultaneously on the ESP32 at all times (when in normal mode).
The phone decides which one to use for the current command.

```
  TRANSPORT COMPARISON

  Property          BLE                WiFi AP           4G Relay
  --------          ---                -------           --------
  Range             ~10m               ~50m open air     unlimited
  Latency           20 - 80ms          5 - 20ms          50 - 250ms
  Power (ESP32)     30 - 50mA          200 - 300mA       200 - 300mA
  Requires router   no                 no                VPS + internet
  iOS support       no (Apple blocked) yes               yes
  Android Chrome    yes                yes               yes
  When to use       close range, saver normal operation  remote control


  SELECTION LOGIC (runs in phone browser JavaScript)

  On connect:
  1. Does browser support Web Bluetooth?
       NO  --> skip to step 3
       YES --> try BLE scan for "MSRP1-Rover"
  2. BLE found?
       NO  --> try step 3
       YES --> connect BLE, use as primary transport
  3. Try WebSocket to ws://192.168.4.1/ws (ESP32 AP)
       SUCCESS --> use WiFi as transport
       FAIL    --> try step 4
  4. Is relay URL configured?
       NO  --> show "not connected" error
       YES --> connect to VPS WebSocket (wss://VPS_IP:8081/?role=phone)
               phone bridges VPS <-> ESP32 (BLE or WiFi, whichever works)

  If active transport drops:
       BLE drops  --> immediately try WiFi path
       WiFi drops --> immediately try 4G relay path
       All drop   --> motor watchdog fires after 500ms, motors stop


  DATA FLOW: DRIVE COMMAND (PC to motors)

  PC browser                 VPS                 Phone browser
  ----------                 ---                 -------------
  user moves joystick
  |
  v
  JSON: {type:"drive",       ----websocket--->   receives command
         left:180,right:180}                     |
                                                 v
                                                 (which transport is active?)
                                                  |
                             +---BLE path---------+---WiFi path----------+
                             |                                            |
                             v                                            v
                      BLE UART write                           WebSocket send
                      to ESP32 RX char                         to 192.168.4.1/ws
                             |                                            |
                             +--------------------+------------------------+
                                                  |
                                                  v
                                           processCommand()
                                           updates g_cmd struct
                                                  |
                                           taskMotorController
                                           reads g_cmd at 100Hz
                                                  |
                                           setMotors(left, right)
                                                  |
                                           TB6612 drives motors


  DATA FLOW: TELEMETRY (rover to PC)

  ESP32 taskTelemetry (10Hz)
  |
  +-- reads INA219 (voltage, current, power)
  +-- reads DS18B20 (temperature)
  |
  v
  builds JSON string in stack buffer (no heap allocation)
  |
  +-- if WiFi clients connected: ws.textAll(buffer)
  +-- if BLE client connected:   bleTxChar->notify(buffer)
  |
  v
  Phone browser receives on whichever transport it used
  |
  +-- if in relay mode: forward to VPS websocket
  |
  v
  VPS forwards to PC browser
  |
  v
  PC browser updates telemetry display
```

---

## 6. Software Architecture

### Firmware task layout (FreeRTOS)

The ESP32 runs four concurrent tasks. Tasks on Core 1 are time-critical.
Tasks on Core 0 can tolerate occasional delays from WiFi/BLE interrupts.

```
  Core 1 (time-critical)         Core 0 (background)
  ----------------------         -------------------
  taskMotorController            taskTelemetry
  priority 5 (highest)           priority 3
  period: 10ms (100Hz)           period: 100ms (10Hz)
  stack: 4096 bytes              stack: 5120 bytes
  |                              |
  reads g_cmd (mutex)            reads INA219, DS18B20
  applies dead zone (+/-10)      writes g_telem (mutex)
  applies watchdog (500ms)       broadcasts JSON via WiFi + BLE
  calls setMotors()              checks auto battery saver
                                 |
                                 taskFanController
                                 priority 2
                                 period: 1000ms (1Hz)
                                 stack: 2048 bytes
                                 |
                                 hysteresis: on@40C, off@35C
                                 disabled in battery saver mode
                                 |
                                 taskOLED
                                 priority 1 (lowest)
                                 period: 500ms (2Hz)
                                 stack: 4096 bytes
                                 |
                                 renders: transport, voltage,
                                 current, temp, fan, battery bar
                                 skipped in battery saver mode


  SHARED DATA PROTECTION

  g_cmd struct (left, right speeds):
    written by: WebSocket handler, BLE RX callback
    read by:    taskMotorController
    protected:  cmdMutex (FreeRTOS mutex)

  g_telem struct (voltage, current, temp, fan, clients):
    written by: taskTelemetry, taskFanController
    read by:    taskOLED, HTTP status endpoint
    protected:  telemMutex (FreeRTOS mutex)

  g_powerMode (POWER_NORMAL / POWER_SAVER):
    written by: processCommand(), taskTelemetry (auto)
    read by:    taskFanController, taskOLED, taskTelemetry
    type:       volatile enum (no mutex, single-word atomic write)


  COMMAND PROCESSING PATH

  WebSocket onWsEvent()                BLE BLERxCallbacks::onWrite()
       |                                        |
       v                                        v
  data[len] = 0   (null terminate)    chr->getValue() (std::string)
       |                                        |
       +------------------+--------------------+
                          |
                          v
                  processCommand(json, transport_source)
                          |
                  deserializeJson() via ArduinoJson
                          |
             +------------+------------+
             |            |            |
          "drive"       "stop"       "power"
             |            |            |
        update g_cmd  zero g_cmd   set g_powerMode
        update        update        (saver/normal)
        g_lastCmdMs   g_lastCmdMs
        g_lastTransport
```

### File structure and what each file does

```
  rover-v3/
  |
  +-- esp32/
  |   +-- rover_v3_firmware.ino    Single .ino file, all firmware
  |                                 Flashed to ESP32 via Arduino IDE
  |                                 Contains: pin defs, tasks, BLE server,
  |                                 WebSocket server, embedded HTML fallback
  |
  +-- phone/
  |   +-- index.html               Single HTML file, no build step
  |                                 Opened in Chrome Android on phone
  |                                 Contains: transport manager, joystick,
  |                                 BLE via Web Bluetooth API,
  |                                 WiFi via WebSocket API,
  |                                 camera via getUserMedia API,
  |                                 GPS via Geolocation API,
  |                                 IMU via DeviceOrientation API
  |
  +-- server/
  |   +-- cloud_relay.js           Node.js server, runs on VPS
  |   +-- package.json             npm dependencies: ws, express
  |
  +-- SETUP_V3.md                  This file
```

---

## 7. Firmware Setup (ESP32)

### 7.1 Install Arduino IDE and ESP32 board support

Download Arduino IDE 2.x from arduino.cc. Open File -> Preferences and add
this URL to "Additional boards manager URLs":

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Go to Tools -> Board -> Boards Manager. Search "esp32". Install
"esp32 by Espressif Systems" version 2.x or later.

### 7.2 Install required libraries

Tools -> Manage Libraries. Search and install each:

```
  Library name (search exactly)    Author              Purpose
  -----------------------------    ------              -------
  NimBLE-Arduino                   h2zero              BLE server (NEW in V3)
  ESPAsyncWebServer                lacamera            HTTP + WebSocket server
  AsyncTCP                         dvarrel             Required by above
  Adafruit SSD1306                 Adafruit            OLED display driver
  Adafruit GFX Library             Adafruit            Required by above
  Adafruit INA219                  Adafruit            Voltage/current sensor
  DallasTemperature                Miles Burton        DS18B20 temperature
  OneWire                          Paul Stoffregen     Required by above
  ArduinoJson                      Benoit Blanchon     JSON parse and build

  Note: ESPAsyncWebServer and AsyncTCP may not appear in Library Manager.
  If not found, download as ZIP from GitHub and install via:
  Sketch -> Include Library -> Add .ZIP Library

  ESPAsyncWebServer: https://github.com/lacamera/ESPAsyncWebServer
  AsyncTCP:          https://github.com/dvarrel/AsyncTCP
```

### 7.3 Board settings

```
  Setting              Value
  -------              -----
  Board                ESP32 Dev Module
  Upload Speed         921600
  CPU Frequency        240 MHz
  Flash Frequency      80 MHz
  Flash Mode           QIO
  Flash Size           4MB (32Mb)
  Partition Scheme     Default 4MB with spiffs
  Core Debug Level     None
  Port                 COMx (Windows) or /dev/ttyUSB0 (Linux/Mac)
```

### 7.4 Flash procedure

1. Open rover_v3_firmware.ino in Arduino IDE
2. Select correct board and port (Tools menu)
3. Click Upload (right arrow button)
4. If upload fails with "Failed to connect": hold the BOOT button on the
   ESP32 while clicking Upload, release after "Connecting..." appears

### 7.5 Verify boot via Serial Monitor

Open Tools -> Serial Monitor. Set baud rate to 115200. Press the RST button
on the ESP32. You must see all of these lines in order:

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

  If any [WARN] lines appear:
  [WARN] OLED not found at 0x3C, trying 0x3D...  -->  check SDA/SCL wiring + pullups
  [WARN] INA219 not found (0x40)                  -->  check I2C wiring
  [WARN] DS18B20 not found                        -->  check 4.7k pullup on GPIO4
```

---

## 8. Phone Dashboard Setup

### 8.1 Browser requirement

```
  Browser          BLE support   WiFi support   Recommended
  -------          -----------   ------------   -----------
  Chrome Android   YES           YES            PRIMARY CHOICE
  Firefox Android  no            YES            WiFi only
  Safari iOS       no            YES            WiFi only
  Chrome iOS       no            YES            Apple blocks BLE in all iOS browsers
  Chrome Desktop   YES*          YES            *requires HTTPS or localhost
```

### 8.2 Serving the dashboard to the phone

Option A — serve from PC on local network (easiest for development):

```
  On PC, in the rover-v3/phone/ folder, run:
    python3 -m http.server 8080

  On phone, open Chrome and go to:
    http://YOUR_PC_LAN_IP:8080

  Find your PC LAN IP:
    Windows:  ipconfig  (look for IPv4 Address under WiFi adapter)
    Linux:    ip addr   (look for inet under wlan0 or eth0)
    Mac:      ifconfig  (look for inet under en0)
```

Option B — serve from the rover itself (no PC needed):

```
  The ESP32 serves a minimal embedded dashboard at http://192.168.4.1
  This is the fallback. It has full joystick + telemetry + power controls.
  Connect phone to ROVER-V3 WiFi, then open 192.168.4.1 in any browser.
  Camera, GPS, and IMU are not available in the embedded version.
```

Option C — host index.html on VPS (for permanent remote access):

```
  Copy phone/index.html to your VPS web root:
    scp phone/index.html user@VPS_IP:/var/www/html/rover.html

  Access from anywhere at:
    http://VPS_IP/rover.html
```

### 8.3 First BLE connection (Chrome Android)

1. Open Chrome on Android. Go to Settings -> Site settings -> Bluetooth.
   Make sure it is allowed.
2. Open the dashboard URL.
3. Click "Connect BLE" button. Chrome shows a device picker.
4. Select "MSRP1-Rover" from the list.
5. Click Pair. The connection dot turns green.
6. Telemetry values start updating. Joystick now controls motors.

After the first pairing, subsequent connections are automatic. The browser
remembers the device and reconnects without showing the picker.

### 8.4 Transport mode switching

```
  Dashboard transport indicator states:

  "BLE" (green dot)   -- BLE active, lowest power, phone within ~10m
  "WiFi" (green dot)  -- WiFi WebSocket active, phone on ROVER-V3 network
  "Relay" (green dot) -- 4G relay active, VPS URL configured and connected
  "---" (red dot)     -- no transport active, motors will stop in 500ms

  Switching is automatic. To force a specific mode, use the mode buttons
  in the dashboard (DIRECT / LOCAL WiFi / RELAY).
```

---

## 9. VPS Relay Setup

The VPS relay is only needed for controlling the rover from home while the
rover is somewhere else on a phone data connection. Skip this section for
local operation.

### 9.1 VPS requirements

```
  Minimum spec:    1 vCPU, 512MB RAM, 1GB storage
  Recommended:     1 vCPU, 1GB RAM (DigitalOcean $4/mo, Linode $5/mo)
  OS:              Ubuntu 22.04 LTS
  Open ports:      8081/tcp (WebSocket relay)
  Node.js version: 18 or later
```

### 9.2 Install Node.js on VPS

```bash
ssh user@YOUR_VPS_IP

curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

node --version   # should print v18.x.x or later
```

### 9.3 Upload relay server

```bash
# From your PC, in the rover-v3/ folder:
scp -r server/ user@YOUR_VPS_IP:~/msrp1/

# On VPS:
cd ~/msrp1
npm install
```

### 9.4 Open firewall

```bash
sudo ufw allow 8081/tcp
sudo ufw status   # verify rule appears
```

### 9.5 Run the relay server

```bash
# Test run (stops when you close SSH):
node cloud_relay.js

# Persistent run (keeps running after SSH closes):
npm install -g pm2
pm2 start cloud_relay.js --name rover-relay
pm2 save
pm2 startup   # follow the printed command to auto-start on reboot
```

Expected output:

```
[SERVER] MSRP-1 V3 Relay running
  Dashboard:   http://0.0.0.0:8081
  Rover WS:    ws://0.0.0.0:8081/?role=rover
  PC WS:       ws://0.0.0.0:8081/?role=pc
  Phone WS:    ws://0.0.0.0:8081/?role=phone
```

### 9.6 Configure phone dashboard to use relay

In phone/index.html, find this section and replace YOUR_VPS_IP:

```javascript
const MODE_URLS = {
  direct: 'ws://192.168.4.1/ws',
  lan:    'ws://192.168.1.100/ws',
  relay:  'wss://YOUR_VPS_IP:8081/?role=phone'   // <-- edit this
};
```

### 9.7 Relay data flow diagram

```
  PC browser                   VPS cloud_relay.js             Phone browser
  ----------                   ------------------             -------------
  connects to:                 receives role=pc               connects to:
  wss://VPS:8081/?role=pc      stores as connections.pc       wss://VPS:8081/?role=phone
                                                              stores as connections.phone
  user pushes joystick
       |
       v
  ws.send({type:"drive"...})
       |
       v                       receives from pc
                               if connections.phone:
                                 phone.send(data)
                                       |
                                       v
                               receives from phone
                                       |
                              (phone decides: BLE or WiFi)
                                       |
                         +------------+------------+
                         |                         |
                  BLE write to                WS send to
                  ESP32 RX char               192.168.4.1/ws
                         |                         |
                         +------------+------------+
                                      |
                               processCommand()
                                      |
                               taskMotorController
                                      |
                               motors move

  Telemetry travels the reverse path:
  ESP32 -> phone (BLE notify or WS message) -> VPS -> PC display
```

---

## 10. Battery Saver Mode

### What shuts down and what stays on

```
  Component        Normal mode          Battery saver mode
  ---------        -----------          ------------------
  WiFi AP          ON (150-200mA)       OFF (WiFi.mode(WIFI_OFF))
  WebSocket srv    ON                   OFF (no WiFi)
  OLED display     ON (20mA)            OFF (SSD1306_DISPLAYOFF cmd)
  Fan              ON (temp-controlled) OFF (forced LOW)
  BLE server       ON (30mA)            ON (stays active)
  INA219           ON (1mA)             ON (stays active)
  DS18B20          ON (1mA)             ON (stays active)
  Motor drivers    ON                   ON (you still drive)
  ESP32 core       ON                   ON (no deep sleep, must respond)


  TRIGGER CONDITIONS

  Manual:
    phone sends: {"type":"power","mode":"saver"}
    received via BLE or WiFi, calls g_powerMode = POWER_SAVER

  Automatic:
    taskTelemetry detects voltage < 7.0V (approx 20% charge)
    prints: [PWR] Low battery (x.xxV) -> auto battery saver
    calls:  g_powerMode = POWER_SAVER

  RECOVERY

  Wake command via BLE (only path available in saver mode):
    phone sends: {"type":"power","mode":"normal"}
    firmware calls applyPowerMode(POWER_NORMAL)
    WiFi AP restarts, OLED turns on, fan control resumes

  If BLE connection is lost in saver mode:
    motor watchdog fires after 500ms
    motors stop
    rover is stationary but still broadcasting BLE advertisements
    reconnect with phone to recover
```

### Power saver transition diagram

```
  NORMAL MODE
  WiFi ON + BLE ON + OLED ON + Fan controlled
       |                              ^
       | trigger:                     | command: {"type":"power","mode":"normal"}
       | voltage < 7.0V              | via BLE only (WiFi is off)
       | OR manual command            |
       v                              |
  BATTERY SAVER MODE
  WiFi OFF + BLE ON + OLED OFF + Fan OFF
  current draw: ~30-50mA (not counting motors)
```

---

## 11. Communication Protocol Reference

All messages are UTF-8 JSON strings. The same format is used on both BLE
and WiFi transports. There is no binary framing.

### Commands (phone/PC to ESP32)

```
  Drive:
  {"type":"drive","left":180,"right":180}
    left:  -255 to 255  (negative = reverse, 0 = brake)
    right: -255 to 255
    Must be sent continuously at 100ms intervals.
    Motor watchdog stops motors if no command for 500ms.

  Stop (immediate brake):
  {"type":"stop"}
    Sets both motors to active brake immediately.
    Resets watchdog timer.

  Power mode:
  {"type":"power","mode":"saver"}
  {"type":"power","mode":"normal"}

  Ping (latency check):
  {"type":"ping"}
    ESP32 replies: {"type":"pong"}
    on the same transport the ping arrived on.
```

### Telemetry (ESP32 to phone/PC, every 100ms)

```json
{
  "type":      "telem",
  "v":         7.82,
  "mA":        340,
  "mW":        2659,
  "temp":      31.5,
  "fan":       false,
  "wifiCl":    1,
  "ble":       true,
  "transport": "ble",
  "power":     "normal"
}
```

```
  Field        Type     Range/values          Meaning
  -----        ----     ------------          -------
  v            float    6.0 - 8.4             Battery voltage in Volts
  mA           float    0 - 3200*             Current draw in milliamps
  mW           float    0 - ~26000*           Power in milliwatts
  temp         float    -10 to 80             Electronics temp in Celsius
  fan          bool     true/false            Fan currently on
  wifiCl       int      0 - N                 Connected WiFi WebSocket clients
  ble          bool     true/false            BLE client connected
  transport    string   "ble"/"wifi"/"none"   Transport of last drive command
  power        string   "normal"/"saver"      Current power mode

  * INA219 saturates at 3200mA. Under heavy motor load readings clip at this
    value. This is a hardware limitation of the 0.1Ohm onboard shunt resistor.
    To fix: replace shunt with 0.01Ohm for 10x range (soldering required).
```

### BLE characteristics

```
  Service UUID:  6E400001-B5A3-F393-E0A9-E50E24DCCA9E  (Nordic UART Service)

  RX characteristic: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
    Phone WRITES commands here.
    Properties: WRITE, WRITE_NR (no response, for speed)

  TX characteristic: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
    ESP32 sends telemetry here via NOTIFY.
    Phone must subscribe to notifications on connect.
    Properties: NOTIFY

  MTU: NimBLE negotiates with the phone automatically.
  Typical result: 247 bytes MTU on Android Chrome.
  Telemetry JSON is ~150 bytes, safely under limit.
```

---

## 12. Build and Test Sequence

Build in this order. Do not proceed to the next step until the current
step passes. Each step has a specific, observable pass/fail criterion.

```
  Step  What to build                   Pass criterion
  ----  -------------                   --------------
  1     Flash firmware, no wiring       Serial Monitor shows all [BOOT] lines
                                        No [WARN] lines (sensors not yet wired)

  2     Wire and test OLED alone        OLED shows "MSRP-1 V3 / Booting..."
        (I2C SDA SCL pullups first)     i2c_scanner shows device at 0x3C or 0x3D

  3     Wire INA219 in series           Serial Monitor shows realistic voltage
        on positive power rail          (7.x V for charged 2S pack)
                                        i2c_scanner shows device at 0x40

  4     Wire DS18B20 with pullup        Serial Monitor shows ~25-30C room temp
                                        -127C means pullup resistor missing

  5     Wire ONE motor to TB6612#1      Motor spins forward then brakes in loop
        (STBY, AIN1, AIN2, PWMA)       If it twitches: PWM not zeroed before STBY HIGH
                                        If wrong direction: swap AO1/AO2 wires

  6     Wire remaining 3 motors         All 4 motors spin same direction on "forward"
                                        command. Verify no motor runs backwards.

  7     Wire fan MOSFET                 Fan turns on when DS18B20 held in fingers
        (100R series, 10k pulldown)     (~36C). Verify fan is OFF at cold boot.

  8     Connect to ROVER-V3 WiFi        Browser at 192.168.4.1 loads dashboard
        from phone                      Telemetry updates. Joystick moves rover.
                                        Watchdog test: close browser, rover
                                        must stop within 500ms.

  9     BLE test (Chrome Android)       Transport indicator shows "BLE"
                                        Telemetry updates via BLE notify
                                        Joystick moves rover via BLE path

  10    Battery saver test              WiFi AP disappears from network list
        Press "Battery Saver" button    OLED goes dark
                                        BLE stays connected
                                        Joystick still moves rover
                                        Press "Normal": WiFi AP reappears

  11    VPS relay test (optional)       From a different network (mobile data),
                                        open relay URL in browser
                                        Telemetry updates. Joystick moves rover.
                                        Latency will be higher (50-250ms).
```

---

## 13. Fault Diagnosis

### ESP32 does not appear as a serial port

```
  Symptom: no COM port or /dev/ttyUSB0 appears when connected via USB
  Cause A: cable is charge-only (no data lines)
           Fix: try a different USB cable
  Cause B: CP2102 or CH340 driver not installed
           Fix: download driver for your OS from silabs.com or wch.cn
  Cause C: wrong USB port on a hub
           Fix: connect directly to PC USB port
```

### Upload fails with "Failed to connect to ESP32"

```
  Hold the BOOT button on ESP32 during upload.
  Release after "Connecting..." appears in Arduino IDE output.
  This manually forces bootloader mode.
```

### Motor does not move

```
  Check in order:
  1. Serial Monitor shows "Motor drivers enabled"?
       NO  --> firmware crash in setup. Add Serial.println() before each
               pinMode to find where it stops.
  2. Is STBY pin (GPIO33) HIGH after boot?
       Measure with multimeter. Should be 3.3V.
       NO  --> firmware not setting it HIGH. Check ledcWrite(0) before STBY HIGH.
  3. Is TB6612 VCC (3.3V logic supply) present?
       Measure VM pin: should equal battery voltage.
       Measure VCC pin: should be 3.3V.
  4. Does AIN1 or AIN2 toggle when command is sent?
       Measure with multimeter or oscilloscope while sending drive command.
       NO  --> pin assignment mismatch in firmware defines.
```

### INA219 reads 0V or 0mA

```
  Cause A: wired in parallel instead of series
           The current must flow THROUGH the INA219 chip (VIN+ to VIN-).
           Check: disconnect VIN- from rest of circuit and measure continuity
           from battery + through INA219 to motor driver VM.
  Cause B: VIN+ and VIN- reversed
           Fix: swap the two rails going into INA219.
  Cause C: I2C address wrong
           Run i2c_scanner. INA219 should appear at 0x40.
           If not found: check SDA SCL pullup resistors.
```

### DS18B20 reads -127C

```
  This specific value means "sensor not found on 1-Wire bus".
  Cause: 4.7k pullup resistor from GPIO4 to 3.3V is missing.
  The 1-Wire protocol requires this resistor to function.
  Without it the data line never reaches logic HIGH.
  Fix: solder 4.7k between GPIO4 and 3.3V rail.
```

### OLED shows nothing

```
  Step 1: Run i2c_scanner. Does any address appear?
           NO  --> pullup resistors on SDA/SCL are missing.
                   Add 4.7k from GPIO21 to 3.3V and GPIO22 to 3.3V.
           YES --> go to step 2.
  Step 2: Is the address 0x3C or 0x3D?
           OTHER --> wrong device (check if INA219 is on wrong address)
  Step 3: Does firmware Serial Monitor show "[BOOT] OLED OK"?
           NO, shows "[WARN] OLED not found" --> VCC not connected or
           wrong I2C address for this module variant.
```

### ESP32 resets when motors start

```
  Cause: motors pull large startup current, causing voltage sag on 5V rail,
         which browses the ESP32.
  Fix A: add 1000uF electrolytic capacitor from TB6612 VM to GND,
         physically close to the TB6612 chip.
  Fix B: add 100uF + 0.1uF on LM2596 output before ESP32 VIN.
  Fix C: check that 18AWG wire is used for motor power lines, not thinner.
```

### BLE not found in Chrome Android

```
  Step 1: Phone Bluetooth is on?
  Step 2: Chrome Android version >= 56?
  Step 3: Site permissions: Settings -> Site settings -> Bluetooth -> Allow?
  Step 4: Is ESP32 within range (~10m)?
  Step 5: Serial Monitor shows "[BOOT] BLE advertising as 'MSRP1-Rover'"?
           NO --> NimBLE-Arduino library not installed correctly.
                  Delete library folder and reinstall from Library Manager.
  Step 6: Try scanning with nRF Connect app (free, Android) to confirm
          ESP32 is visible at BLE level before testing browser.
```

### Motors run backwards on one side

```
  This is a wiring polarity issue, not a firmware issue.
  Swap the two motor wires at the TB6612 output (AO1/AO2 or BO1/BO2).
  Do not modify direction logic in firmware to compensate.
  Physical wire swap is the correct fix.
```

### Watchdog fires immediately (motors stop right after command)

```
  Cause: drive commands are sent once and not repeated.
  The firmware requires continuous drive commands every 100ms.
  The phone dashboard send loop runs every 100ms and keeps the
  watchdog satisfied. If you are writing custom control code,
  you must send {"type":"drive",...} at least every 400ms.
```

---



*-------------------------------------------------------------------------------------End-----------------------------------------------------------------------------*
