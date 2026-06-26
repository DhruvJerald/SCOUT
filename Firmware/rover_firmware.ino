/*
*config
*setup
*loop
*websocket handler
*motor control
*html page for dashboard
*
 */

 #include <Wifi.h>
 #include <ESPAsyncWebServer.h>
 #include <AsyncWebsocket.h>

 const char*WIFI_SSD="SCOUT";
 const char*WIFI_PASSWORD="SCOUT123";

 #define LEFT_FORWARD 25
 #define LEFT_BACKWARD 26
 #define LEFT_SPEED 27

 #define LEFT_REAR_FORWARD 14
 #define LEFT_REAR_BACKWARD 12
 #define LEFT_REAR_SPEED 13

#define RIGHT_FORWARD  32   
#define RIGHT_BACKWARD 2    
#define RIGHT_SPEED    23   

#define RIGHT_REAR_FORWARD  19   
#define RIGHT_REAR_BACKWARD 5    
#define RIGHT_REAR_SPEED    18   


#define STBY_PIN 33

// Current motor speeds (like variables that change)
int leftSpeed = 0;    // -255 to +255 (negative = reverse)
int rightSpeed = 0;   // -255 to +255

// Web server and WebSocket objects
AsyncWebServer server(80);        // HTTP server on port 80
AsyncWebSocket ws("/ws");         // WebSocket at /ws

void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");

  pinMode(LEFT_FORWARD, OUTPUT);
  pinMode(LEFT_BACKWARD, OUTPUT);
  pinMode(LEFT_SPEED, OUTPUT);
  
  pinMode(LEFT_REAR_FORWARD, OUTPUT);
  pinMode(LEFT_REAR_BACKWARD, OUTPUT);
  pinMode(LEFT_REAR_SPEED, OUTPUT);
  
  pinMode(RIGHT_FORWARD, OUTPUT);
  pinMode(RIGHT_BACKWARD, OUTPUT);
  pinMode(RIGHT_SPEED, OUTPUT);
  
  pinMode(RIGHT_REAR_FORWARD, OUTPUT);
  pinMode(RIGHT_REAR_BACKWARD, OUTPUT);
  pinMode(RIGHT_REAR_SPEED, OUTPUT);
  
  pinMode(STBY_PIN, OUTPUT);
  
  // Enable motors (STBY HIGH = motors on)
  digitalWrite(STBY_PIN, HIGH);
  Serial.println("[OK] Motors ready");

  // ── 2. Set up PWM for motor speed ──
  // This is like setting up a dimmer switch for the motors
  ledcSetup(0, 5000, 8);    // Channel 0, 5kHz, 8-bit (0-255)
  ledcAttachPin(LEFT_SPEED, 0);
  
  ledcSetup(1, 5000, 8);
  ledcAttachPin(LEFT_REAR_SPEED, 1);
  
  ledcSetup(2, 5000, 8);
  ledcAttachPin(RIGHT_SPEED, 2);
  
  ledcSetup(3, 5000, 8);
  ledcAttachPin(RIGHT_REAR_SPEED, 3);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[OK] WiFi started: ");
  Serial.println(WIFI_SSID);
  Serial.print("     IP Address: ");
  Serial.println(WiFi.softAPIP());

  // ── 4. Set up WebSocket (for joystick) ──
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // ── 5. Serve the HTML page ──
  // This is like sending index.html when someone visits
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  server.begin();
  Serial.println("Web server has started");
  Serial.println("     Open browser to: http://192.168.4.1");
  Serial.println("=== SCOUT ROVER READY ===");
}

void loop(){
  ws.cleanupClients();
  delay(10);
}

void setMotors(int left, int right) {
  // ── LEFT MOTORS ──
  if (left > 0) {
    // Forward
    digitalWrite(LEFT_FORWARD, HIGH);
    digitalWrite(LEFT_BACKWARD, LOW);
    digitalWrite(LEFT_REAR_FORWARD, HIGH);
    digitalWrite(LEFT_REAR_BACKWARD, LOW);
    ledcWrite(0, left);    // Front left speed
    ledcWrite(1, left);    // Rear left speed
  } else if (left < 0) {
    // Reverse
    digitalWrite(LEFT_FORWARD, LOW);
    digitalWrite(LEFT_BACKWARD, HIGH);
    digitalWrite(LEFT_REAR_FORWARD, LOW);
    digitalWrite(LEFT_REAR_BACKWARD, HIGH);
    ledcWrite(0, -left);   // Front left speed (positive)
    ledcWrite(1, -left);   // Rear left speed (positive)
  } else {
    // Stop (brake)
    digitalWrite(LEFT_FORWARD, HIGH);
    digitalWrite(LEFT_BACKWARD, HIGH);
    digitalWrite(LEFT_REAR_FORWARD, HIGH);
    digitalWrite(LEFT_REAR_BACKWARD, HIGH);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
  }

  // ── RIGHT MOTORS ──
  if (right > 0) {
    digitalWrite(RIGHT_FORWARD, HIGH);
    digitalWrite(RIGHT_BACKWARD, LOW);
    digitalWrite(RIGHT_REAR_FORWARD, HIGH);
    digitalWrite(RIGHT_REAR_BACKWARD, LOW);
    ledcWrite(2, right);
    ledcWrite(3, right);
  } else if (right < 0) {
    digitalWrite(RIGHT_FORWARD, LOW);
    digitalWrite(RIGHT_BACKWARD, HIGH);
    digitalWrite(RIGHT_REAR_FORWARD, LOW);
    digitalWrite(RIGHT_REAR_BACKWARD, HIGH);
    ledcWrite(2, -right);
    ledcWrite(3, -right);
  } else {
    digitalWrite(RIGHT_FORWARD, HIGH);
    digitalWrite(RIGHT_BACKWARD, HIGH);
    digitalWrite(RIGHT_REAR_FORWARD, HIGH);
    digitalWrite(RIGHT_REAR_BACKWARD, HIGH);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
  }
}
void setMotors(int left, int right) {
  // ── LEFT MOTORS ──
  if (left > 0) {
    // Forward
    digitalWrite(LEFT_FORWARD, HIGH);
    digitalWrite(LEFT_BACKWARD, LOW);
    digitalWrite(LEFT_REAR_FORWARD, HIGH);
    digitalWrite(LEFT_REAR_BACKWARD, LOW);
    ledcWrite(0, left);    // Front left speed
    ledcWrite(1, left);    // Rear left speed
  } else if (left < 0) {
    // Reverse
    digitalWrite(LEFT_FORWARD, LOW);
    digitalWrite(LEFT_BACKWARD, HIGH);
    digitalWrite(LEFT_REAR_FORWARD, LOW);
    digitalWrite(LEFT_REAR_BACKWARD, HIGH);
    ledcWrite(0, -left);   // Front left speed (positive)
    ledcWrite(1, -left);   // Rear left speed (positive)
  } else {
    // Stop (brake)
    digitalWrite(LEFT_FORWARD, HIGH);
    digitalWrite(LEFT_BACKWARD, HIGH);
    digitalWrite(LEFT_REAR_FORWARD, HIGH);
    digitalWrite(LEFT_REAR_BACKWARD, HIGH);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
  }

  // ── RIGHT MOTORS ──
  if (right > 0) {
    digitalWrite(RIGHT_FORWARD, HIGH);
    digitalWrite(RIGHT_BACKWARD, LOW);
    digitalWrite(RIGHT_REAR_FORWARD, HIGH);
    digitalWrite(RIGHT_REAR_BACKWARD, LOW);
    ledcWrite(2, right);
    ledcWrite(3, right);
  } else if (right < 0) {
    digitalWrite(RIGHT_FORWARD, LOW);
    digitalWrite(RIGHT_BACKWARD, HIGH);
    digitalWrite(RIGHT_REAR_FORWARD, LOW);
    digitalWrite(RIGHT_REAR_BACKWARD, HIGH);
    ledcWrite(2, -right);
    ledcWrite(3, -right);
  } else {
    digitalWrite(RIGHT_FORWARD, HIGH);
    digitalWrite(RIGHT_BACKWARD, HIGH);
    digitalWrite(RIGHT_REAR_FORWARD, HIGH);
    digitalWrite(RIGHT_REAR_BACKWARD, HIGH);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
  }
}

void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  
  if (type == WS_EVT_DATA) {
    // When the browser sends joystick data, it comes as a string
    // Example: "200,180" means left=200, right=180
    // We need to split this into two numbers
    
    String message = String((char*)data);
    message = message.substring(0, len);  // Clean up
    
    // Find where the comma is
    int commaPos = message.indexOf(',');
    
    if (commaPos != -1) {
      // Extract left and right speeds
      String leftStr = message.substring(0, commaPos);
      String rightStr = message.substring(commaPos + 1);
      
      // Convert strings to numbers
      int left = leftStr.toInt();
      int right = rightStr.toInt();
      
      // Save the speeds
      leftSpeed = left;
      rightSpeed = right;
      
      // Apply to motors
      setMotors(leftSpeed, rightSpeed);
      
      // Debug print (you'll see this in serial monitor)
      Serial.print("Left: ");
      Serial.print(left);
      Serial.print("  Right: ");
      Serial.println(right);
    }
  }
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>Scout Rover</title>
  <style>
    /* ── CSS (You already know this!) ── */
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: #0a0a0a;
      color: #00ff88;
      font-family: 'Courier New', monospace;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      padding: 20px;
      gap: 20px;
    }
    h1 {
      color: #00ff88;
      font-size: 2rem;
      letter-spacing: 4px;
      text-shadow: 0 0 20px #00ff8840;
    }
    .status {
      font-size: 0.8rem;
      color: #555;
    }
    .status.connected {
      color: #00ff88;
    }
    
    /* Joystick */
    .joy-wrap {
      position: relative;
      width: 200px;
      height: 200px;
      touch-action: none;
    }
    .joy-base {
      width: 200px;
      height: 200px;
      border-radius: 50%;
      background: #1a1a2e;
      border: 2px solid #00ff88;
      box-shadow: 0 0 40px #00ff8820;
    }
    .joy-thumb {
      position: absolute;
      width: 60px;
      height: 60px;
      background: #00ff88;
      border-radius: 50%;
      top: 70px;
      left: 70px;
      cursor: grab;
      box-shadow: 0 0 30px #00ff8860;
      transition: background 0.1s;
    }
    .joy-thumb:active {
      background: #00cc66;
      cursor: grabbing;
    }
    
    .controls {
      display: flex;
      gap: 30px;
      align-items: center;
      flex-wrap: wrap;
      justify-content: center;
    }
    button {
      background: #1a1a2e;
      border: 1px solid #00ff88;
      color: #00ff88;
      padding: 12px 24px;
      border-radius: 8px;
      cursor: pointer;
      font-family: inherit;
      font-size: 1rem;
      transition: all 0.2s;
    }
    button:active {
      background: #00ff8833;
    }
    .stop-btn {
      border-color: #ff4444;
      color: #ff4444;
    }
    .stop-btn:active {
      background: #ff444433;
    }
    small {
      color: #555;
      font-size: 0.7rem;
    }
    #telemetry {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 10px;
      width: 100%;
      max-width: 400px;
    }
    .tcard {
      background: #1a1a2e;
      border: 1px solid #333;
      border-radius: 8px;
      padding: 10px;
      text-align: center;
    }
    .tcard .label {
      font-size: 0.6rem;
      color: #555;
      text-transform: uppercase;
    }
    .tcard .value {
      font-size: 1.1rem;
      color: #00ff88;
      font-weight: bold;
    }
  </style>
</head>
<body>

  <h1>SCOUT ROVER</h1>
  <div id="status" class="status">● Connecting...</div>

  <div id="telemetry">
    <div class="tcard"><div class="label">Left</div><div class="value" id="leftVal">0</div></div>
    <div class="tcard"><div class="label">Right</div><div class="value" id="rightVal">0</div></div>
    <div class="tcard"><div class="label">Status</div><div class="value" id="statusVal">Ready</div></div>
    <div class="tcard"><div class="label">Mode</div><div class="value" id="modeVal">WiFi</div></div>
  </div>

  <div class="controls">
    <div class="joy-wrap" id="joyWrap">
      <div class="joy-base"></div>
      <div class="joy-thumb" id="joyThumb"></div>
    </div>
    <div>
      <button id="stopBtn" class="stop-btn">■ STOP</button>
    </div>
  </div>

  <small>Connect to WiFi: ROVER-V3 | Open: 192.168.4.1</small>

  <script>
  // ──────────────────────────────────────────────────────────────
  // JAVASCRIPT (You already know this!)
  // ──────────────────────────────────────────────────────────────

  let ws = null;
  let leftSpeed = 0;
  let rightSpeed = 0;

  // ── WebSocket connection ──
  function connectWebSocket() {
    // Connect to the ESP32's WebSocket
    ws = new WebSocket('ws://' + location.hostname + '/ws');
    
    ws.onopen = function() {
      document.getElementById('status').textContent = '● Connected';
      document.getElementById('status').className = 'status connected';
      document.getElementById('statusVal').textContent = 'Online';
    };
    
    ws.onclose = function() {
      document.getElementById('status').textContent = '● Disconnected';
      document.getElementById('status').className = 'status';
      document.getElementById('statusVal').textContent = 'Offline';
      // Try to reconnect after 2 seconds
      setTimeout(connectWebSocket, 2000);
    };
    
    ws.onerror = function() {
      document.getElementById('statusVal').textContent = 'Error';
    };
  }

  // ── Send command to ESP32 ──
  function sendCommand(left, right) {
    // This sends: "left,right" as a string
    // Example: "200,180" means left=200, right=180
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(left + ',' + right);
      
      // Update display
      document.getElementById('leftVal').textContent = left;
      document.getElementById('rightVal').textContent = right;
    }
  }

  // ── Emergency stop ──
  function emergencyStop() {
    leftSpeed = 0;
    rightSpeed = 0;
    sendCommand(0, 0);
    document.getElementById('statusVal').textContent = 'STOPPED';
    setTimeout(function() {
      document.getElementById('statusVal').textContent = 'Ready';
    }, 1000);
  }

  // ── Joystick (touch + mouse) ──
  const wrap = document.getElementById('joyWrap');
  const thumb = document.getElementById('joyThumb');
  const MAX_RADIUS = 70;  // Max distance from center

  function handleJoyMove(cx, cy) {
    // Get joystick center position
    const rect = wrap.getBoundingClientRect();
    const centerX = rect.left + rect.width / 2;
    const centerY = rect.top + rect.height / 2;
    
    // Calculate distance from center
    let dx = cx - centerX;
    let dy = cy - centerY;
    let dist = Math.sqrt(dx*dx + dy*dy);
    
    // Limit to joystick radius
    if (dist > MAX_RADIUS) {
      dx = dx / dist * MAX_RADIUS;
      dy = dy / dist * MAX_RADIUS;
      dist = MAX_RADIUS;
    }
    
    // Move thumb
    thumb.style.transform = 'translate(' + dx + 'px, ' + dy + 'px)';
    
    // Calculate motor speeds
    // y: up = forward, down = backward
    // x: left = turn left, right = turn right
    const maxSpeed = 200;
    let forward = -dy / MAX_RADIUS;  // -1 to +1
    let turn = dx / MAX_RADIUS;      // -1 to +1
    
    // Differential steering (tank drive)
    let left = Math.round((forward + turn) * maxSpeed);
    let right = Math.round((forward - turn) * maxSpeed);
    
    // Limit to valid range
    left = Math.max(-maxSpeed, Math.min(maxSpeed, left));
    right = Math.max(-maxSpeed, Math.min(maxSpeed, right));
    
    // Send command
    sendCommand(left, right);
  }

  function handleJoyRelease() {
    thumb.style.transform = 'translate(0, 0)';
    sendCommand(0, 0);
  }

  // ── Touch events ──
  wrap.addEventListener('touchstart', function(e) {
    e.preventDefault();
    const touch = e.touches[0];
    handleJoyMove(touch.clientX, touch.clientY);
  }, {passive: false});

  wrap.addEventListener('touchmove', function(e) {
    e.preventDefault();
    const touch = e.touches[0];
    handleJoyMove(touch.clientX, touch.clientY);
  }, {passive: false});

  wrap.addEventListener('touchend', function(e) {
    e.preventDefault();
    handleJoyRelease();
  }, {passive: false});

  wrap.addEventListener('touchcancel', function(e) {
    e.preventDefault();
    handleJoyRelease();
  }, {passive: false});

  // ── Mouse events (for desktop testing) ──
  let isDragging = false;

  wrap.addEventListener('mousedown', function(e) {
    isDragging = true;
    handleJoyMove(e.clientX, e.clientY);
  });

  document.addEventListener('mousemove', function(e) {
    if (isDragging) {
      handleJoyMove(e.clientX, e.clientY);
    }
  });

  document.addEventListener('mouseup', function() {
    if (isDragging) {
      isDragging = false;
      handleJoyRelease();
    }
  });

  // ── Stop button ──
  document.getElementById('stopBtn').addEventListener('click', emergencyStop);

  // ── Connect when page loads ──
  connectWebSocket();
  </script>
</body>
</html>
)rawliteral";
