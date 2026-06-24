/**
 * MSRP-1 Rover V3 Firmware
 * ESP32 DevKit V1
 *
 * Transport stack (all run simultaneously):
 *   1. BLE UART server (NimBLE, Nordic UART Service)   — primary, lowest power
 *   2. WiFi AP + WebSocket server                       — secondary, fastest throughput
 *
 * The phone decides which transport to use. Commands arriving on either path
 * are handled identically by the motor controller task.
 *
 * Battery saver mode (triggered by command or low voltage):
 *   - WiFi OFF, OLED OFF, Fan OFF
 *   - Only BLE + motors + INA219 remain active
 *   - ~30–50mA total vs ~200–300mA normal
 *
 * Libraries (Arduino Library Manager):
 *   NimBLE-Arduino       by h2zero          ← NEW for BLE
 *   ESPAsyncWebServer    by lacamera
 *   AsyncTCP             by dvarrel
 *   Adafruit SSD1306     by Adafruit
 *   Adafruit GFX Library by Adafruit
 *   Adafruit INA219      by Adafruit
 *   DallasTemperature    by Miles Burton
 *   OneWire              by Paul Stoffregen
 *   ArduinoJson          by Benoit Blanchon
 */

#include <Arduino.h>
#include <Wire.h>

// ── BLE ──────────────────────────────────────────────────────────────────
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

// ── WiFi + WebSocket ──────────────────────────────────────────────────────
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

// ── JSON ──────────────────────────────────────────────────────────────────
#include <ArduinoJson.h>

// ── Sensors + Display ─────────────────────────────────────────────────────
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ═════════════════════════════════════════════════════════════════════════
// CONFIGURATION — edit these before flashing
// ═════════════════════════════════════════════════════════════════════════

const char* AP_SSID      = "ROVER-V3";
const char* AP_PASSWORD  = "rover1234";      // min 8 chars; "" = open network
const char* BLE_NAME     = "MSRP1-Rover";   // Appears in phone Bluetooth scan

// Safety: stop motors if no command within this window (either transport)
const uint32_t WATCHDOG_MS = 500;

// Fan hysteresis (°C)
const float FAN_ON_TEMP  = 40.0f;
const float FAN_OFF_TEMP = 35.0f;

// Battery thresholds (2S Li-ion)
const float BAT_MIN_VOLTAGE  = 6.0f;    // 2 × 3.0V — BMS cutoff
const float BAT_MAX_VOLTAGE  = 8.4f;    // 2 × 4.2V — full charge
const float BAT_SAVER_VOLTS  = 7.0f;    // <7.0V (~20%) → auto battery saver

// ═════════════════════════════════════════════════════════════════════════
// PIN DEFINITIONS
// ═════════════════════════════════════════════════════════════════════════

// TB6612 #1 — Left side
#define L_F_AIN1   25
#define L_F_AIN2   26
#define L_F_PWMA   27   // LEDC ch 0
#define L_R_BIN1   14
#define L_R_BIN2   12
#define L_R_PWMB   13   // LEDC ch 1
#define TB1_STBY   33   // Shared STBY for both TB6612s

// TB6612 #2 — Right side
#define R_F_AIN1   32
#define R_F_AIN2    2   // Moved from GPIO15 (strapping pin)
#define R_F_PWMA   23   // LEDC ch 2
#define R_R_BIN1   19
#define R_R_BIN2    5
#define R_R_PWMB   18   // LEDC ch 3  (moved from GPIO17, reserved for UART)

// Fan MOSFET (AO3400 N-ch)
// Hardware required: 100Ω series gate resistor + 10kΩ pull-down gate→GND
#define FAN_PIN    15

// DS18B20 — 4.7kΩ pull-up to 3.3V required
#define DS18B20_PIN 4

// Reserved: GPIO16=UART2_RX, GPIO17=UART2_TX (future GPS)
// I2C: GPIO21=SDA, GPIO22=SCL
// Flash: GPIO6–11 — NEVER USE

// LEDC
#define LEDC_CH_LF    0
#define LEDC_CH_LR    1
#define LEDC_CH_RF    2
#define LEDC_CH_RR    3
#define LEDC_FREQ  5000
#define LEDC_RES      8

// ═════════════════════════════════════════════════════════════════════════
// NimBLE UUIDs  (Nordic UART Service — standard, works with most BLE apps)
// ═════════════════════════════════════════════════════════════════════════

#define BLE_SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_RX_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Phone writes here
#define BLE_CHAR_TX_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 notifies here

// ═════════════════════════════════════════════════════════════════════════
// OBJECTS
// ═════════════════════════════════════════════════════════════════════════

AsyncWebServer   httpServer(80);
AsyncWebSocket   wsServer("/ws");

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Adafruit_INA219  ina219;
OneWire          oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

// BLE objects
NimBLEServer*         bleServer       = nullptr;
NimBLECharacteristic* bleTxChar       = nullptr;  // ESP32 → Phone notifications
bool                  bleConnected    = false;

// ═════════════════════════════════════════════════════════════════════════
// SHARED STATE
// ═════════════════════════════════════════════════════════════════════════

struct MotorCommand { int left; int right; };

struct Telemetry {
  float voltage;
  float current_mA;
  float power_mW;
  float temperature;
  bool  fanOn;
  int   wifiClients;
  bool  bleClient;
};

// Transport tracking
enum Transport { TRANSPORT_NONE, TRANSPORT_BLE, TRANSPORT_WIFI };

volatile MotorCommand g_cmd         = {0, 0};
volatile Telemetry    g_telem       = {0,0,0,0,false,0,false};
volatile uint32_t     g_lastCmdMs   = 0;
volatile Transport    g_lastTransport = TRANSPORT_NONE;

// Power mode
enum PowerMode { POWER_NORMAL, POWER_SAVER };
volatile PowerMode    g_powerMode   = POWER_NORMAL;

SemaphoreHandle_t cmdMutex;
SemaphoreHandle_t telemMutex;

// ═════════════════════════════════════════════════════════════════════════
// MOTOR HELPERS
// ═════════════════════════════════════════════════════════════════════════

void driveMotor(uint8_t in1, uint8_t in2, uint8_t ch, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
    ledcWrite(ch, (uint8_t)speed);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);  digitalWrite(in2, HIGH);
    ledcWrite(ch, (uint8_t)(-speed));
  } else {
    digitalWrite(in1, HIGH); digitalWrite(in2, HIGH);   // Active brake
    ledcWrite(ch, 0);
  }
}

void setMotors(int l, int r) {
  driveMotor(L_F_AIN1, L_F_AIN2, LEDC_CH_LF, l);
  driveMotor(L_R_BIN1, L_R_BIN2, LEDC_CH_LR, l);
  driveMotor(R_F_AIN1, R_F_AIN2, LEDC_CH_RF, r);
  driveMotor(R_R_BIN1, R_R_BIN2, LEDC_CH_RR, r);
}

// ═════════════════════════════════════════════════════════════════════════
// COMMAND PROCESSOR  (shared by BLE and WiFi paths)
// Both transports call this with raw JSON. One place, no duplication.
// ═════════════════════════════════════════════════════════════════════════

void processCommand(const char* json, Transport src) {
  StaticJsonDocument<192> doc;
  if (deserializeJson(doc, json)) return;   // Malformed JSON — ignore

  const char* type = doc["type"];
  if (!type) return;

  if (strcmp(type, "drive") == 0) {
    int l = constrain((int)doc["left"],  -255, 255);
    int r = constrain((int)doc["right"], -255, 255);
    xSemaphoreTake(cmdMutex, portMAX_DELAY);
      g_cmd.left  = l;
      g_cmd.right = r;
      g_lastCmdMs = millis();
      g_lastTransport = src;
    xSemaphoreGive(cmdMutex);

  } else if (strcmp(type, "stop") == 0) {
    xSemaphoreTake(cmdMutex, portMAX_DELAY);
      g_cmd.left  = 0;
      g_cmd.right = 0;
      g_lastCmdMs = millis();
    xSemaphoreGive(cmdMutex);

  } else if (strcmp(type, "power") == 0) {
    // {"type":"power","mode":"saver"} or {"type":"power","mode":"normal"}
    const char* mode = doc["mode"];
    if (mode) {
      if (strcmp(mode, "saver") == 0)  g_powerMode = POWER_SAVER;
      if (strcmp(mode, "normal") == 0) g_powerMode = POWER_NORMAL;
    }

  } else if (strcmp(type, "ping") == 0) {
    // Reply on the same transport
    const char* pong = "{\"type\":\"pong\"}";
    if (src == TRANSPORT_BLE && bleTxChar && bleConnected) {
      bleTxChar->setValue((uint8_t*)pong, strlen(pong));
      bleTxChar->notify();
    }
    // WiFi pong is sent in the WS event handler directly
  }
}

// ═════════════════════════════════════════════════════════════════════════
// BLE SERVER CALLBACKS
// ═════════════════════════════════════════════════════════════════════════

class BLEServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* srv) override {
    bleConnected = true;
    xSemaphoreTake(telemMutex, portMAX_DELAY);
      g_telem.bleClient = true;
    xSemaphoreGive(telemMutex);
    Serial.println("[BLE] Client connected");
  }
  void onDisconnect(NimBLEServer* srv) override {
    bleConnected = false;
    xSemaphoreTake(telemMutex, portMAX_DELAY);
      g_telem.bleClient = false;
    xSemaphoreGive(telemMutex);
    Serial.println("[BLE] Client disconnected, restarting advertising");
    NimBLEDevice::startAdvertising();
  }
};

class BLERxCallbacks : public NimBLECharacteristicCallbacks {
  // Called when phone writes to the RX characteristic
  void onWrite(NimBLECharacteristic* chr) override {
    std::string val = chr->getValue();
    if (val.length() > 0) {
      processCommand(val.c_str(), TRANSPORT_BLE);
    }
  }
};

// ═════════════════════════════════════════════════════════════════════════
// WEBSOCKET HANDLER  (WiFi path)
// ═════════════════════════════════════════════════════════════════════════

void onWsEvent(AsyncWebSocket* srv, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WS] Client #%u from %s\n", client->id(),
                  client->remoteIP().toString().c_str());
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"connected\",\"ssid\":\"%s\"}", AP_SSID);
    client->text(buf);

  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WS] Client #%u disconnected\n", client->id());

  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len
        && info->opcode == WS_TEXT) {
      data[len] = '\0';

      // Handle ping here so we can reply on the same client socket
      if (strstr((char*)data, "\"ping\"")) {
        client->text("{\"type\":\"pong\"}");
      } else {
        processCommand((char*)data, TRANSPORT_WIFI);
      }
    }
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("[WS] Error: %s\n", (char*)data);
  }
}

// ═════════════════════════════════════════════════════════════════════════
// POWER MODE MANAGEMENT
// ═════════════════════════════════════════════════════════════════════════

PowerMode g_prevMode = POWER_NORMAL;

void applyPowerMode(PowerMode mode) {
  if (mode == POWER_SAVER) {
    // Shut down WiFi — saves 100–200mA
    if (WiFi.getMode() != WIFI_OFF) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      Serial.println("[PWR] WiFi OFF — battery saver active");
    }
    // OLED off — saves ~20mA
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    // Fan off
    digitalWrite(FAN_PIN, LOW);
    Serial.println("[PWR] OLED + Fan OFF");

  } else {
    // Restore WiFi
    if (WiFi.getMode() == WIFI_OFF) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASSWORD);
      Serial.printf("[PWR] WiFi AP restored: %s\n", WiFi.softAPIP().toString().c_str());
    }
    // OLED on
    oled.ssd1306_command(SSD1306_DISPLAYON);
    Serial.println("[PWR] Normal mode restored");
  }
}

// ═════════════════════════════════════════════════════════════════════════
// TASK 1: Motor Controller  (100 Hz, Core 1)
// ═════════════════════════════════════════════════════════════════════════

void taskMotorController(void* _) {
  const TickType_t period   = pdMS_TO_TICKS(10);
  TickType_t       lastWake = xTaskGetTickCount();

  while (true) {
    uint32_t now = millis();
    int l, r;

    xSemaphoreTake(cmdMutex, portMAX_DELAY);
      l = g_cmd.left;
      r = g_cmd.right;
      uint32_t lastCmd = g_lastCmdMs;
    xSemaphoreGive(cmdMutex);

    // Watchdog — stop if command is stale on any transport
    if ((now - lastCmd) > WATCHDOG_MS) { l = 0; r = 0; }

    // Dead zone — suppress joystick drift noise
    if (abs(l) < 10) l = 0;
    if (abs(r) < 10) r = 0;

    setMotors(l, r);
    vTaskDelayUntil(&lastWake, period);
  }
}

// ═════════════════════════════════════════════════════════════════════════
// TASK 2: Telemetry  (10 Hz, Core 0)
// Reads sensors, broadcasts on whichever transports are active
// ═════════════════════════════════════════════════════════════════════════

void taskTelemetry(void* _) {
  const TickType_t period   = pdMS_TO_TICKS(100);
  TickType_t       lastWake = xTaskGetTickCount();

  while (true) {
    float v   = ina219.getBusVoltage_V();
    float mA  = ina219.getCurrent_mA();
    float mW  = ina219.getPower_mW();
    tempSensor.requestTemperatures();
    float t   = tempSensor.getTempCByIndex(0);
    int   wifiCl = wsServer.count();
    bool  bleCl  = bleConnected;

    xSemaphoreTake(telemMutex, portMAX_DELAY);
      g_telem.voltage     = v;
      g_telem.current_mA  = mA;
      g_telem.power_mW    = mW;
      g_telem.temperature = t;
      g_telem.wifiClients = wifiCl;
      g_telem.bleClient   = bleCl;
    xSemaphoreGive(telemMutex);

    // Auto battery saver: triggers if voltage drops below threshold
    if (v > 0.5f && v < BAT_SAVER_VOLTS && g_powerMode == POWER_NORMAL) {
      Serial.printf("[PWR] Low battery (%.2fV) → auto battery saver\n", v);
      g_powerMode = POWER_SAVER;
    }

    // Handle power mode transitions
    PowerMode mode = g_powerMode;
    if (mode != g_prevMode) {
      applyPowerMode(mode);
      g_prevMode = mode;
    }

    // Build telemetry JSON
    // Stack buffer — no heap allocation
    char buf[300];
    Transport lastTr;
    xSemaphoreTake(cmdMutex, portMAX_DELAY);
      lastTr = g_lastTransport;
    xSemaphoreGive(cmdMutex);

    const char* trName = (lastTr == TRANSPORT_BLE)  ? "ble"  :
                         (lastTr == TRANSPORT_WIFI) ? "wifi" : "none";
    const char* pwrName = (mode == POWER_SAVER) ? "saver" : "normal";

    snprintf(buf, sizeof(buf),
      "{\"type\":\"telem\","
      "\"v\":%.2f,\"mA\":%.0f,\"mW\":%.0f,\"temp\":%.1f,"
      "\"fan\":%s,\"wifiCl\":%d,\"ble\":%s,"
      "\"transport\":\"%s\",\"power\":\"%s\"}",
      v, mA, mW, t,
      g_telem.fanOn ? "true" : "false",
      wifiCl,
      bleCl ? "true" : "false",
      trName, pwrName
    );

    // Send on WiFi WebSocket
    if (wifiCl > 0 && mode == POWER_NORMAL) {
      wsServer.textAll(buf);
    }

    // Send on BLE (notify) — only if connected
    // BLE MTU is typically 20–240 bytes. Our payload is ~150 bytes.
    // NimBLE negotiates a larger MTU automatically with modern phones.
    if (bleCl && bleTxChar) {
      bleTxChar->setValue((uint8_t*)buf, strlen(buf));
      bleTxChar->notify();
    }

    vTaskDelayUntil(&lastWake, period);
  }
}

// ═════════════════════════════════════════════════════════════════════════
// TASK 3: Fan Controller  (1 Hz, Core 0)
// ═════════════════════════════════════════════════════════════════════════

void taskFanController(void* _) {
  const TickType_t period   = pdMS_TO_TICKS(1000);
  TickType_t       lastWake = xTaskGetTickCount();
  bool fanState = false;

  while (true) {
    // Fan is disabled in power saver mode
    if (g_powerMode == POWER_SAVER) {
      if (fanState) { digitalWrite(FAN_PIN, LOW); fanState = false; }
      vTaskDelayUntil(&lastWake, period);
      continue;
    }

    float temp;
    xSemaphoreTake(telemMutex, portMAX_DELAY);
      temp = g_telem.temperature;
    xSemaphoreGive(telemMutex);

    if (!fanState && temp >= FAN_ON_TEMP)  fanState = true;
    if (fanState  && temp <= FAN_OFF_TEMP) fanState = false;

    digitalWrite(FAN_PIN, fanState ? HIGH : LOW);

    xSemaphoreTake(telemMutex, portMAX_DELAY);
      g_telem.fanOn = fanState;
    xSemaphoreGive(telemMutex);

    vTaskDelayUntil(&lastWake, period);
  }
}

// ═════════════════════════════════════════════════════════════════════════
// TASK 4: OLED Display  (2 Hz, Core 0)
// ═════════════════════════════════════════════════════════════════════════

void taskOLED(void* _) {
  const TickType_t period   = pdMS_TO_TICKS(500);
  TickType_t       lastWake = xTaskGetTickCount();

  while (true) {
    // Skip render in battery saver (display already off via applyPowerMode)
    if (g_powerMode == POWER_SAVER) {
      vTaskDelayUntil(&lastWake, period);
      continue;
    }

    Telemetry t;
    xSemaphoreTake(telemMutex, portMAX_DELAY);
      t = g_telem;
    xSemaphoreGive(telemMutex);

    Transport tr;
    xSemaphoreTake(cmdMutex, portMAX_DELAY);
      tr = g_lastTransport;
    xSemaphoreGive(cmdMutex);

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);

    // Row 0: Active transport indicator
    oled.setCursor(0, 0);
    oled.print("Link: ");
    oled.print(tr == TRANSPORT_BLE  ? "BLE"  :
               tr == TRANSPORT_WIFI ? "WiFi" : "none");

    // Row 1: Connections
    oled.setCursor(0, 10);
    oled.print("W:");
    oled.print(t.wifiClients);
    oled.print(" B:");
    oled.print(t.bleClient ? "1" : "0");

    // Row 2: Voltage
    oled.setCursor(0, 20);
    oled.print("V:  "); oled.print(t.voltage, 2); oled.print("V");

    // Row 3: Current
    oled.setCursor(0, 30);
    oled.print("I:  "); oled.print(t.current_mA, 0); oled.print("mA");

    // Row 4: Temp + Fan
    oled.setCursor(0, 40);
    oled.print("T:  "); oled.print(t.temperature, 1);
    oled.print("C F:"); oled.print(t.fanOn ? "Y" : "N");

    // Row 5: Power mode
    oled.setCursor(0, 52);
    oled.print("PWR: NORMAL");

    // Battery bar (top right, 40px wide)
    float pct = constrain(
      (t.voltage - BAT_MIN_VOLTAGE) / (BAT_MAX_VOLTAGE - BAT_MIN_VOLTAGE),
      0.0f, 1.0f
    );
    oled.drawRect(88, 0, 40, 8, SSD1306_WHITE);
    oled.fillRect(88, 0, (int)(40 * pct), 8, SSD1306_WHITE);

    oled.display();
    vTaskDelayUntil(&lastWake, period);
  }
}

// ═════════════════════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[BOOT] MSRP-1 V3 starting...");

  cmdMutex   = xSemaphoreCreateMutex();
  telemMutex = xSemaphoreCreateMutex();

  // ── Motor direction pins ───────────────────────────────────────────────
  uint8_t dirPins[] = {
    L_F_AIN1, L_F_AIN2, L_R_BIN1, L_R_BIN2,
    R_F_AIN1, R_F_AIN2, R_R_BIN1, R_R_BIN2
  };
  for (uint8_t p : dirPins) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }

  // ── STBY LOW during init ───────────────────────────────────────────────
  pinMode(TB1_STBY, OUTPUT);
  digitalWrite(TB1_STBY, LOW);

  // ── LEDC PWM setup — all zero BEFORE enabling STBY ────────────────────
  ledcSetup(LEDC_CH_LF, LEDC_FREQ, LEDC_RES); ledcAttachPin(L_F_PWMA, LEDC_CH_LF);
  ledcSetup(LEDC_CH_LR, LEDC_FREQ, LEDC_RES); ledcAttachPin(L_R_PWMB, LEDC_CH_LR);
  ledcSetup(LEDC_CH_RF, LEDC_FREQ, LEDC_RES); ledcAttachPin(R_F_PWMA, LEDC_CH_RF);
  ledcSetup(LEDC_CH_RR, LEDC_FREQ, LEDC_RES); ledcAttachPin(R_R_PWMB, LEDC_CH_RR);
  for (int ch = 0; ch < 4; ch++) ledcWrite(ch, 0);

  // Now safe to enable motor drivers
  digitalWrite(TB1_STBY, HIGH);
  Serial.println("[BOOT] Motor drivers enabled");

  // ── Fan pin (10kΩ pull-down to GND required on hardware) ─────────────
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  // ── I2C ───────────────────────────────────────────────────────────────
  Wire.begin(21, 22);

  // ── OLED (try 0x3C, fallback 0x3D) ────────────────────────────────────
  bool oledOk = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledOk) {
    Serial.println("[WARN] OLED not at 0x3C, trying 0x3D...");
    oledOk = oled.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  }
  if (oledOk) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0,0); oled.println("MSRP-1 V3");
    oled.setCursor(0,12); oled.println("Booting...");
    oled.display();
    Serial.println("[BOOT] OLED OK");
  } else {
    Serial.println("[WARN] OLED not found at 0x3C or 0x3D");
  }

  // ── INA219 ────────────────────────────────────────────────────────────
  if (!ina219.begin()) {
    Serial.println("[WARN] INA219 not found (0x40). Check wiring.");
  } else {
    ina219.setCalibration_32V_2A();   // Clips at 3.2A — see hardware notes
    Serial.println("[BOOT] INA219 OK");
  }

  // ── DS18B20 ───────────────────────────────────────────────────────────
  tempSensor.begin();
  if (tempSensor.getDeviceCount() == 0) {
    Serial.println("[WARN] DS18B20 not found. Check 4.7kΩ pull-up on GPIO4.");
  } else {
    tempSensor.setResolution(11);
    Serial.printf("[BOOT] DS18B20 OK (%d sensor)\n", tempSensor.getDeviceCount());
  }

  // ── NimBLE Server ─────────────────────────────────────────────────────
  NimBLEDevice::init(BLE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);   // Max TX power

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new BLEServerCallbacks());

  NimBLEService* bleService = bleServer->createService(BLE_SERVICE_UUID);

  // RX characteristic — phone writes commands here
  NimBLECharacteristic* bleRxChar = bleService->createCharacteristic(
    BLE_CHAR_RX_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  bleRxChar->setCallbacks(new BLERxCallbacks());

  // TX characteristic — ESP32 sends telemetry here via notify
  bleTxChar = bleService->createCharacteristic(
    BLE_CHAR_TX_UUID,
    NIMBLE_PROPERTY::NOTIFY
  );

  bleService->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(BLE_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();
  Serial.printf("[BOOT] BLE advertising as '%s'\n", BLE_NAME);

  // ── WiFi AP ───────────────────────────────────────────────────────────
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("[BOOT] WiFi AP: SSID='%s'  IP=%s\n",
                AP_SSID, WiFi.softAPIP().toString().c_str());

  // ── WebSocket ─────────────────────────────────────────────────────────
  wsServer.onEvent(onWsEvent);
  httpServer.addHandler(&wsServer);

  // ── HTTP — serve embedded dashboard ───────────────────────────────────
  httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", ROVER_HTML);
  });
  httpServer.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    Telemetry t;
    xSemaphoreTake(telemMutex, portMAX_DELAY); t = g_telem; xSemaphoreGive(telemMutex);
    char buf[256];
    snprintf(buf, sizeof(buf),
      "{\"v\":%.2f,\"mA\":%.0f,\"temp\":%.1f,\"power\":\"%s\",\"ble\":%s,\"wifiCl\":%d}",
      t.voltage, t.current_mA, t.temperature,
      g_powerMode == POWER_SAVER ? "saver" : "normal",
      t.bleClient ? "true" : "false", t.wifiClients
    );
    req->send(200, "application/json", buf);
  });
  httpServer.begin();
  Serial.println("[BOOT] HTTP + WebSocket server on port 80");

  // ── FreeRTOS Tasks ────────────────────────────────────────────────────
  xTaskCreatePinnedToCore(taskMotorController, "Motors", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(taskTelemetry,       "Telem",  5120, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(taskFanController,   "Fan",    2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskOLED,            "OLED",   4096, NULL, 1, NULL, 0);

  Serial.println("[BOOT] All tasks started. Rover ready.");
  Serial.println("[BOOT] Transports: BLE (primary) + WiFi AP (secondary)");
}

void loop() {
  wsServer.cleanupClients();
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ═════════════════════════════════════════════════════════════════════════
// EMBEDDED MINIMAL DASHBOARD  (WiFi direct access at 192.168.4.1)
// The full dashboard is phone/index.html — this is a fallback for direct WiFi.
// ═════════════════════════════════════════════════════════════════════════

const char ROVER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>MSRP-1 V3</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0a0a0f;color:#d0d0e0;font-family:'Courier New',monospace;
  display:flex;flex-direction:column;align-items:center;padding:12px;gap:10px}
h1{color:#00ff88;letter-spacing:4px;font-size:1.3rem}
.dot{width:10px;height:10px;border-radius:50%;background:#ff4455;display:inline-block}
.dot.ok{background:#00ff88;box-shadow:0 0 6px #00ff88}
.tg{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;width:100%;max-width:400px}
.tc{background:#12121f;border:1px solid #2a2a3f;border-radius:8px;padding:8px;text-align:center}
.tl{font-size:.6rem;color:#555}
.tv{font-size:1rem;color:#00ff88;font-weight:bold}
.jw{position:relative;width:140px;height:140px;touch-action:none;user-select:none}
.jb{width:100%;height:100%;border-radius:50%;background:#12121f;border:2px solid #2a2a3f}
.jt{position:absolute;width:48px;height:48px;background:#00ff88;border-radius:50%;
  top:46px;left:46px;pointer-events:none;box-shadow:0 0 10px #00ff88}
.ctrl{display:flex;gap:16px;align-items:center}
.btns{display:grid;grid-template-columns:1fr 1fr;gap:6px}
button{background:#12121f;border:1px solid #2a2a3f;color:#d0d0e0;
  padding:10px;border-radius:8px;cursor:pointer;font-family:inherit;font-size:.8rem}
button:active{background:#00ff8822;border-color:#00ff88}
.es{background:#2e0a0a;border-color:#ff4455;color:#ff4455;grid-column:span 2;font-weight:bold}
.es:active{background:#ff445533}
.pwr{display:flex;gap:6px;width:100%;max-width:400px}
.pwr button{flex:1}
.pwr button.active{background:#00ff8820;border-color:#00ff88;color:#00ff88}
small{color:#444;font-size:.6rem}
</style></head><body>
<h1>MSRP-1 V3</h1>
<div style="display:flex;gap:8px;align-items:center;font-size:.7rem;color:#555">
  <span id="cl">●</span><span id="cs">Connecting...</span>
</div>
<div class="tg">
  <div class="tc"><div class="tl">VOLTAGE</div><div class="tv" id="tv">--</div></div>
  <div class="tc"><div class="tl">CURRENT</div><div class="tv" id="ti">--</div></div>
  <div class="tc"><div class="tl">POWER</div><div class="tv" id="tp">--</div></div>
  <div class="tc"><div class="tl">TEMP</div><div class="tv" id="tt">--</div></div>
  <div class="tc"><div class="tl">TRANSPORT</div><div class="tv" id="tr">--</div></div>
  <div class="tc"><div class="tl">MODE</div><div class="tv" id="pm">--</div></div>
</div>
<div class="pwr">
  <button id="btnNorm" class="active" onclick="setPower('normal')">⚡ Normal</button>
  <button id="btnSavr" onclick="setPower('saver')">🔋 Battery Saver</button>
</div>
<div class="ctrl">
  <div class="jw" id="jw"><div class="jb"></div><div class="jt" id="jt"></div></div>
  <div class="btns">
    <button onpointerdown="sc(1,0)" onpointerup="sc(0,0)">▲</button>
    <button onpointerdown="sc(0,1)" onpointerup="sc(0,0)">↻</button>
    <button onpointerdown="sc(0,-1)" onpointerup="sc(0,0)">↺</button>
    <button onpointerdown="sc(-1,0)" onpointerup="sc(0,0)">▼</button>
    <button class="es" onclick="estop()">⬛ E-STOP</button>
  </div>
</div>
<small>WiFi: ROVER-V3 | 192.168.4.1 | V3</small>
<script>
let ws,si,L=0,R=0,spd=200;
function conn(){
  ws=new WebSocket('ws://'+location.hostname+'/ws');
  ws.onopen=()=>{
    document.getElementById('cl').style.color='#00ff88';
    document.getElementById('cs').textContent='Connected (WiFi)';
    si=setInterval(()=>{if(ws.readyState===1)ws.send(JSON.stringify({type:'drive',left:L,right:R}));},100);
  };
  ws.onclose=()=>{
    document.getElementById('cl').style.color='#ff4455';
    document.getElementById('cs').textContent='Disconnected';
    clearInterval(si);setTimeout(conn,2000);
  };
  ws.onmessage=e=>{
    try{const d=JSON.parse(e.data);
      if(d.type==='telem'){
        document.getElementById('tv').textContent=d.v.toFixed(2)+'V';
        document.getElementById('ti').textContent=Math.round(d.mA)+'mA';
        document.getElementById('tp').textContent=(d.mW/1000).toFixed(2)+'W';
        document.getElementById('tt').textContent=d.temp.toFixed(1)+'°C';
        document.getElementById('tr').textContent=(d.transport||'--').toUpperCase();
        document.getElementById('pm').textContent=(d.power||'--').toUpperCase();
        const isSaver=d.power==='saver';
        document.getElementById('btnNorm').className=isSaver?'':'active';
        document.getElementById('btnSavr').className=isSaver?'active':'';
      }
    }catch(e){}
  };
}
function estop(){L=0;R=0;ws&&ws.readyState===1&&ws.send(JSON.stringify({type:'stop'}));}
function sc(f,t){L=Math.round((f+t)*spd);R=Math.round((f-t)*spd);}
function setPower(m){ws&&ws.readyState===1&&ws.send(JSON.stringify({type:'power',mode:m}));}
// Joystick
(function(){
  const w=document.getElementById('jw'),th=document.getElementById('jt'),R2=46,D=8;
  function move(cx,cy){
    const rc=w.getBoundingClientRect();
    let dx=cx-rc.left-rc.width/2,dy=cy-rc.top-rc.height/2;
    const d=Math.sqrt(dx*dx+dy*dy);
    if(d>R2){dx=dx/d*R2;dy=dy/d*R2;}
    th.style.transform=`translate(${dx}px,${dy}px)`;
    const fwd=-dy/R2,turn=dx/R2;
    L=Math.max(-255,Math.min(255,Math.round((fwd+turn)*spd)));
    R=Math.max(-255,Math.min(255,Math.round((fwd-turn)*spd)));
  }
  function rel(){th.style.transform='translate(0,0)';L=0;R=0;}
  w.addEventListener('touchstart',e=>{e.preventDefault();move(e.touches[0].clientX,e.touches[0].clientY);},{passive:false});
  w.addEventListener('touchmove',e=>{e.preventDefault();move(e.touches[0].clientX,e.touches[0].clientY);},{passive:false});
  w.addEventListener('touchend',rel);
  let held=false;
  w.addEventListener('mousedown',e=>{held=true;move(e.clientX,e.clientY);});
  document.addEventListener('mousemove',e=>{if(held)move(e.clientX,e.clientY);});
  document.addEventListener('mouseup',()=>{if(held){held=false;rel();}});
})();
conn();
</script></body></html>
)rawliteral";
