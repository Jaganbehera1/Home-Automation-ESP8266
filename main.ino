#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// WiFi Access Point settings
const char* ap_ssid = "ESP8266_SmartHome";
const char* ap_password = "12345678";

const int LIGHT_PIN = 5;          // GPIO5 (D1)
const int FAN_PIN = LED_BUILTIN;  // GPIO2 (D4) - Built-in LED

const int LIGHT_ADDR = 0;
const int FAN_ADDR = 1;

ESP8266WebServer server(80);

bool lightState = false;
bool fanState = false;

void loadStates();
void saveState(int address, bool state);
void setLight(bool state);
void setFan(bool state);
void setupAP();
void setupServer();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting Smart Home Controller...");

  // Initialize pins
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  // Initialize EEPROM
  EEPROM.begin(512);
  loadStates();

  // Set initial states
  digitalWrite(LIGHT_PIN, lightState ? LOW : HIGH);
  digitalWrite(FAN_PIN, fanState ? LOW : HIGH);

  // Setup Access Point
  setupAP();

  // Setup Web Server
  setupServer();

  Serial.println("Setup complete!");
  Serial.println("Connect to WiFi: " + String(ap_ssid));
  Serial.println("Password: " + String(ap_password));
  Serial.println("Then open: http://192.168.4.1");
}

void setupAP() {
  WiFi.mode(WIFI_AP);
  bool result = WiFi.softAP(ap_ssid, ap_password);

  if (result) {
    Serial.println("Access Point Started Successfully!");
    Serial.print("SSID: ");
    Serial.println(ap_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start Access Point!");
  }
}

void setupServer() {
  // Root page - Control interface
  server.on("/", HTTP_GET, []() {
    String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Home Control</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: #333;
        }

        .container {
            max-width: 800px;
            margin: 0 auto;
        }

        .header {
            text-align: center;
            margin-bottom: 40px;
            color: white;
            animation: fadeInDown 1s ease;
        }

        .header h1 {
            font-size: 2.5rem;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }

        .header p {
            font-size: 1.1rem;
            opacity: 0.9;
        }

        .card-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 25px;
            margin-bottom: 30px;
        }

        .card {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 15px 35px rgba(0,0,0,0.1);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            transition: all 0.3s ease;
            animation: slideInUp 0.8s ease;
            text-align: center;
        }

        .card:hover {
            transform: translateY(-10px);
            box-shadow: 0 20px 40px rgba(0,0,0,0.15);
        }

        .card-title {
            font-size: 1.5rem;
            font-weight: 600;
            color: #2c3e50;
            margin-bottom: 20px;
        }

        .device-container {
            margin: 30px 0;
            position: relative;
            height: 150px;
            display: flex;
            align-items: center;
            justify-content: center;
        }

        /* Bulb Styles */
        .bulb {
            width: 100px;
            height: 100px;
            position: relative;
            transition: all 0.5s ease;
        }

        .bulb-base {
            width: 40px;
            height: 30px;
            background: #888;
            border-radius: 10px 10px 0 0;
            position: absolute;
            bottom: 0;
            left: 50%;
            transform: translateX(-50%);
        }

        .bulb-glass {
            width: 80px;
            height: 80px;
            background: #f0f0f0;
            border-radius: 50%;
            position: absolute;
            bottom: 25px;
            left: 50%;
            transform: translateX(-50%);
            border: 2px solid #ddd;
        }

        .bulb-on .bulb-glass {
            background: #FFD700;
            box-shadow: 0 0 30px #FFD700, 0 0 60px #FFD700;
            animation: glow 2s ease-in-out infinite alternate;
        }

        .bulb-off .bulb-glass {
            background: #f0f0f0;
            opacity: 0.6;
        }

        /* Fan Styles */
        .fan {
            width: 120px;
            height: 120px;
            position: relative;
            transition: all 0.5s ease;
        }

        .fan-center {
            width: 30px;
            height: 30px;
            background: #333;
            border-radius: 50%;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            z-index: 2;
        }

        .fan-blade {
            width: 100px;
            height: 20px;
            background: #00BFFF;
            border-radius: 10px;
            position: absolute;
            top: 50%;
            left: 50%;
            transform-origin: left center;
        }

        .fan-blade-1 { transform: translate(0, -50%) rotate(0deg); }
        .fan-blade-2 { transform: translate(0, -50%) rotate(90deg); }
        .fan-blade-3 { transform: translate(0, -50%) rotate(180deg); }
        .fan-blade-4 { transform: translate(0, -50%) rotate(270deg); }

        .fan-on {
            animation: rotate 1s linear infinite;
        }

        .fan-on .fan-blade {
            background: #0080FF;
        }

        .fan-off .fan-blade {
            background: #888;
            opacity: 0.6;
        }

        .toggle-btn {
            padding: 15px 30px;
            border: none;
            border-radius: 50px;
            font-size: 1.1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            margin: 0 auto;
            min-width: 180px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }

        .btn-on {
            background: linear-gradient(135deg, #00b09b, #96c93d);
            color: white;
        }

        .btn-off {
            background: linear-gradient(135deg, #ff6b6b, #ee5a24);
            color: white;
        }

        .toggle-btn:hover {
            transform: translateY(-3px);
            box-shadow: 0 8px 25px rgba(0,0,0,0.3);
        }

        .toggle-btn:active {
            transform: translateY(-1px);
        }

        .status-text {
            margin-top: 15px;
            font-weight: 600;
            font-size: 1.1rem;
            padding: 10px 20px;
            border-radius: 25px;
            display: inline-block;
        }

        .status-on {
            background: linear-gradient(135deg, #00b09b, #96c93d);
            color: white;
        }

        .status-off {
            background: linear-gradient(135deg, #ff6b6b, #ee5a24);
            color: white;
        }

        .connection-status {
            background: rgba(255, 255, 255, 0.9);
            border-radius: 15px;
            padding: 20px;
            text-align: center;
            animation: fadeIn 1s ease;
        }

        .status-online {
            color: #27ae60;
            font-weight: 600;
        }

        .loading {
            display: none;
            text-align: center;
            margin: 10px 0;
        }

        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #3498db;
            border-radius: 50%;
            width: 30px;
            height: 30px;
            animation: spin 1s linear infinite;
            margin: 0 auto;
        }

        @keyframes fadeInDown {
            from {
                opacity: 0;
                transform: translateY(-30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        @keyframes slideInUp {
            from {
                opacity: 0;
                transform: translateY(30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }

        @keyframes rotate {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }

        @keyframes glow {
            from {
                box-shadow: 0 0 20px #FFD700, 0 0 40px #FFD700;
            }
            to {
                box-shadow: 0 0 30px #FFD700, 0 0 60px #FFD700;
            }
        }

        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.05); }
        }

        .pulse {
            animation: pulse 2s infinite;
        }

        @media (max-width: 768px) {
            .card-grid {
                grid-template-columns: 1fr;
            }
            
            .header h1 {
                font-size: 2rem;
            }
            
            .container {
                padding: 10px;
            }
            
            .bulb, .fan {
                transform: scale(0.8);
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🏠 Smart Home Control</h1>
            <p>Control your devices with one click</p>
        </div>

        <div class="card-grid">
            <!-- Light Card -->
            <div class="card">
                <h2 class="card-title">💡 Light</h2>
                <div class="device-container">
                    <div class="bulb )=====" + String(lightState ? "bulb-on" : "bulb-off") + R"=====(" id="lightIcon">
                        <div class="bulb-glass"></div>
                        <div class="bulb-base"></div>
                    </div>
                </div>
                <div class="loading" id="lightLoading">
                    <div class="spinner"></div>
                    <p>Updating...</p>
                </div>
                <button class="toggle-btn )=====" + String(lightState ? "btn-off" : "btn-on") + R"=====(" onclick="toggleDevice('light')" id="lightBtn">
                    <span id="lightBtnText">)=====" + String(lightState ? "TURN OFF" : "TURN ON") + R"=====(</span>
                </button>
                <div class="status-text )=====" + String(lightState ? "status-on" : "status-off") + R"=====(" id="lightStatus">
                    )=====" + String(lightState ? "LIGHT IS ON" : "LIGHT IS OFF") + R"=====(
                </div>
            </div>

            <!-- Fan Card -->
            <div class="card">
                <h2 class="card-title">🌀 Fan</h2>
                <div class="device-container">
                    <div class="fan )=====" + String(fanState ? "fan-on" : "fan-off") + R"=====(" id="fanIcon">
                        <div class="fan-center"></div>
                        <div class="fan-blade fan-blade-1"></div>
                        <div class="fan-blade fan-blade-2"></div>
                        <div class="fan-blade fan-blade-3"></div>
                        <div class="fan-blade fan-blade-4"></div>
                    </div>
                </div>
                <div class="loading" id="fanLoading">
                    <div class="spinner"></div>
                    <p>Updating...</p>
                </div>
                <button class="toggle-btn )=====" + String(fanState ? "btn-off" : "btn-on") + R"=====(" onclick="toggleDevice('fan')" id="fanBtn">
                    <span id="fanBtnText">)=====" + String(fanState ? "TURN OFF" : "TURN ON") + R"=====(</span>
                </button>
                <div class="status-text )=====" + String(fanState ? "status-on" : "status-off") + R"=====(" id="fanStatus">
                    )=====" + String(fanState ? "FAN IS ON" : "FAN IS OFF") + R"=====(
                </div>
            </div>
        </div>

        <div class="connection-status">
            <p>📶 Connected to ESP8266 Smart Home</p>
            <p><small>Last updated: <span id="lastUpdate">Just now</span></small></p>
        </div>
    </div>

    <script>
        // Initial states from server
        let lightState = )=====" + String(lightState ? "true" : "false") + R"=====(;
        let fanState = )=====" + String(fanState ? "true" : "false") + R"=====(;

        function updateUI() {
            // Update light
            const lightIcon = document.getElementById('lightIcon');
            const lightBtn = document.getElementById('lightBtn');
            const lightBtnText = document.getElementById('lightBtnText');
            const lightStatus = document.getElementById('lightStatus');
            
            if (lightState) {
                lightIcon.className = 'bulb bulb-on pulse';
                lightBtn.className = 'toggle-btn btn-off';
                lightBtnText.textContent = 'TURN OFF';
                lightStatus.textContent = 'LIGHT IS ON';
                lightStatus.className = 'status-text status-on';
            } else {
                lightIcon.className = 'bulb bulb-off';
                lightBtn.className = 'toggle-btn btn-on';
                lightBtnText.textContent = 'TURN ON';
                lightStatus.textContent = 'LIGHT IS OFF';
                lightStatus.className = 'status-text status-off';
            }

            // Update fan
            const fanIcon = document.getElementById('fanIcon');
            const fanBtn = document.getElementById('fanBtn');
            const fanBtnText = document.getElementById('fanBtnText');
            const fanStatus = document.getElementById('fanStatus');
            
            if (fanState) {
                fanIcon.className = 'fan fan-on';
                fanBtn.className = 'toggle-btn btn-off';
                fanBtnText.textContent = 'TURN OFF';
                fanStatus.textContent = 'FAN IS ON';
                fanStatus.className = 'status-text status-on';
            } else {
                fanIcon.className = 'fan fan-off';
                fanBtn.className = 'toggle-btn btn-on';
                fanBtnText.textContent = 'TURN ON';
                fanStatus.textContent = 'FAN IS OFF';
                fanStatus.className = 'status-text status-off';
            }

            // Update timestamp
            document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
        }

        async function toggleDevice(device) {
            const loadingElement = document.getElementById(device + 'Loading');
            const iconElement = document.getElementById(device + 'Icon');
            
            // Show loading
            loadingElement.style.display = 'block';
            iconElement.style.opacity = '0.5';

            try {
                // Determine the action based on current state
                const action = (device === 'light' ? !lightState : !fanState) ? 'on' : 'off';
                const response = await fetch('/' + device + '/' + action);
                const result = await response.text();
                
                // Update state
                if (device === 'light') {
                    lightState = !lightState;
                } else {
                    fanState = !fanState;
                }
                
                // Update UI
                updateUI();
                
                // Show success message
                showNotification(result, 'success');
                
            } catch (error) {
                console.error('Error:', error);
                showNotification('Failed to control ' + device, 'error');
            } finally {
                // Hide loading
                loadingElement.style.display = 'none';
                iconElement.style.opacity = '1';
            }
        }

        function showNotification(message, type) {
            // Create notification element
            const notification = document.createElement('div');
            notification.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                padding: 15px 20px;
                border-radius: 10px;
                color: white;
                font-weight: 600;
                z-index: 1000;
                animation: slideInRight 0.5s ease;
                box-shadow: 0 5px 15px rgba(0,0,0,0.2);
            `;
            
            if (type === 'success') {
                notification.style.background = 'linear-gradient(135deg, #00b09b, #96c93d)';
            } else {
                notification.style.background = 'linear-gradient(135deg, #ff6b6b, #ee5a24)';
            }
            
            notification.textContent = message;
            document.body.appendChild(notification);
            
            // Remove notification after 3 seconds
            setTimeout(() => {
                notification.style.animation = 'slideInRight 0.5s ease reverse';
                setTimeout(() => {
                    document.body.removeChild(notification);
                }, 500);
            }, 3000);
        }

        // Auto-refresh status every 5 seconds
        setInterval(async () => {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                lightState = data.light;
                fanState = data.fan;
                updateUI();
            } catch (error) {
                console.log('Auto-refresh failed:', error);
            }
        }, 5000);

        // Add CSS for notification animation
        const style = document.createElement('style');
        style.textContent = `
            @keyframes slideInRight {
                from {
                    transform: translateX(100%);
                    opacity: 0;
                }
                to {
                    transform: translateX(0);
                    opacity: 1;
                }
            }
        `;
        document.head.appendChild(style);

        // Initial UI update
        updateUI();
    </script>
</body>
</html>
    )=====";
    server.send(200, "text/html", html);
  });

  // Light control endpoints
  server.on("/light/on", HTTP_GET, []() {
    setLight(true);
    server.send(200, "text/plain", "Light turned ON successfully!");
  });

  server.on("/light/off", HTTP_GET, []() {
    setLight(false);
    server.send(200, "text/plain", "Light turned OFF successfully!");
  });

  // Fan control endpoints
  server.on("/fan/on", HTTP_GET, []() {
    setFan(true);
    server.send(200, "text/plain", "Fan turned ON successfully!");
  });

  server.on("/fan/off", HTTP_GET, []() {
    setFan(false);
    server.send(200, "text/plain", "Fan turned OFF successfully!");
  });

  // Status API
  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"light\":" + String(lightState ? "true" : "false") + ",";
    json += "\"fan\":" + String(fanState ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started!");
  Serial.println("Open http://192.168.4.1 in your browser");
}

void loop() {
  server.handleClient();
  delay(10);
}

void setLight(bool state) {
  lightState = state;
  digitalWrite(LIGHT_PIN, state ? LOW : HIGH);
  saveState(LIGHT_ADDR, state);
  Serial.println("Light: " + String(state ? "ON" : "OFF"));
}

void setFan(bool state) {
  fanState = state;
  digitalWrite(FAN_PIN, state ? LOW : HIGH);
  saveState(FAN_ADDR, state);
  Serial.println("Fan: " + String(state ? "ON" : "OFF"));
}

void loadStates() {
  lightState = EEPROM.read(LIGHT_ADDR);
  fanState = EEPROM.read(FAN_ADDR);

  // Validate EEPROM values
  if (lightState != 0 && lightState != 1) lightState = false;
  if (fanState != 0 && fanState != 1) fanState = false;

  Serial.println("Loaded states from EEPROM:");
  Serial.print("Light: ");
  Serial.println(lightState ? "ON" : "OFF");
  Serial.print("Fan: ");
  Serial.println(fanState ? "ON" : "OFF");
}

void saveState(int address, bool state) {
  EEPROM.write(address, state ? 1 : 0);
  if (EEPROM.commit()) {
    Serial.print("Saved state to EEPROM: ");
    Serial.println(state ? "ON" : "OFF");
  } else {
    Serial.println("EEPROM commit failed!");
  }
}
