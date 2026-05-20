#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>

Preferences preferences;
WebServer server(80);

// ================= PINS =================
#define TRIG_PIN 5
#define ECHO_PIN 18
#define PIR_PIN 19

// ================= GLOBALS =================
long duration;
float distance_cm;
int pirState = 0;
int lastPirState = 0;
unsigned long lastMotionTime = 0;
const unsigned long motionDebounce = 2000;

// Config variables loaded from Preferences
String ssid = "";
String password = "";
bool isEnterprise = false;
String eap_id = "";
String eap_username = "";
String eap_password = "";
String server_ip = "";
int server_port = 5003;

bool isAPMode = false;

// ================= HTML PORTAL =================
const char* htmlForm = R"HTML(
<!DOCTYPE html><html><head><title>ESP32 Config</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{font-family:sans-serif;padding:20px;max-width:500px;margin:auto;} 
  input{display:block;margin-bottom:15px;width:100%;padding:10px;box-sizing:border-box;} 
  button{padding:12px;background:#007BFF;color:white;border:none;cursor:pointer;width:100%;font-size:16px;border-radius:5px;}
  .enterprise-only{display:none;}
  h2{text-align:center;}
</style>
<script>
function toggleEnterprise(){
  var isEnt = document.getElementById("ent").checked;
  var els = document.getElementsByClassName("enterprise-only");
  for(var i=0; i<els.length; i++){ els[i].style.display = isEnt ? "block" : "none"; }
}
</script>
</head><body>
<h2>ESP32 Smart Door Config</h2>
<form action="/save" method="POST">
  <label style="font-weight:bold;"><input type="checkbox" id="ent" name="enterprise" value="1" onchange="toggleEnterprise()"> Use WPA2 Enterprise</label><br><br>
  
  <label>Wi-Fi SSID</label>
  <input type="text" name="ssid" required>
  
  <label>Wi-Fi Password (if normal WPA2)</label>
  <input type="password" name="password">
  
  <div class="enterprise-only">
    <label>EAP Identity</label><input type="text" name="eap_id">
    <label>EAP Username</label><input type="text" name="eap_username">
    <label>EAP Password</label><input type="password" name="eap_password">
  </div>
  
  <label>Python Server IP Address</label>
  <input type="text" name="server_ip" placeholder="e.g. 192.168.1.100" required>
  <label>Python Server Port</label>
  <input type="number" name="server_port" value="5003" required>
  
  <button type="submit">Save & Reboot</button>
  <p style="text-align:center;font-size:12px;color:#777;">Wipe settings anytime by pressing EN button while holding BOOT.</p>
</form>
</body></html>
)HTML";

void handleRoot() {
  server.send(200, "text/html", htmlForm);
}

void handleSave() {
  preferences.begin("config", false);
  preferences.putString("ssid", server.arg("ssid"));
  preferences.putString("password", server.arg("password"));
  
  bool isEnt = server.hasArg("enterprise");
  preferences.putBool("enterprise", isEnt);
  
  if (isEnt) {
    preferences.putString("eap_id", server.arg("eap_id"));
    preferences.putString("eap_user", server.arg("eap_username"));
    preferences.putString("eap_pass", server.arg("eap_password"));
  }
  
  preferences.putString("server_ip", server.arg("server_ip"));
  preferences.putInt("server_port", server.arg("server_port").toInt());
  preferences.end();
  
  server.send(200, "text/plain", "Settings Saved! Rebooting ESP32...");
  delay(2000);
  ESP.restart();
}

void startAP() {
  isAPMode = true;
  Serial.println("Starting Captive Portal Access Point...");
  WiFi.softAP("ESP32-Config");
  Serial.print("Connect your device to 'ESP32-Config' and navigate to: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  
  // Load saved credentials
  preferences.begin("config", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  isEnterprise = preferences.getBool("enterprise", false);
  eap_id = preferences.getString("eap_id", "");
  eap_username = preferences.getString("eap_user", "");
  eap_password = preferences.getString("eap_pass", "");
  server_ip = preferences.getString("server_ip", "");
  server_port = preferences.getInt("server_port", 5003);
  preferences.end();
  
  if (ssid == "") {
    Serial.println("No WiFi credentials found.");
    startAP();
  } else {
    Serial.println("Connecting to WiFi: " + ssid);
    WiFi.mode(WIFI_STA);
    
    if (isEnterprise) {
      Serial.println("Using WPA2-Enterprise Auth");
      // Supported on ESP32 Arduino Core 2.0.0+
      WiFi.begin(ssid.c_str(), WPA2_AUTH_PEAP, eap_id.c_str(), eap_username.c_str(), eap_password.c_str());
    } else {
      Serial.println("Using Standard WPA2 Auth");
      WiFi.begin(ssid.c_str(), password.c_str());
    }
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30) {
      delay(500);
      Serial.print(".");
      retries++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nFailed to connect to WiFi! Starting Config AP...");
      startAP();
    } else {
      Serial.println("\nSuccessfully connected to WiFi!");
      Serial.print("ESP32 IP address: ");
      Serial.println(WiFi.localIP());
      
      Serial.println("Waiting 30 seconds for PIR sensor to stabilize...");
      delay(30000);
      Serial.println("System READY. Sending data to " + server_ip + ":" + String(server_port));
    }
  }
}

void loop() {
  if (isAPMode) {
    server.handleClient();
    return;
  }
  
  // 1. Read Ultrasonic
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) distance_cm = 999;
  else distance_cm = duration * 0.034 / 2;
  
  // 2. Read PIR
  pirState = digitalRead(PIR_PIN);
  if (pirState == HIGH && lastPirState == LOW) {
    if (millis() - lastMotionTime > motionDebounce) {
      lastMotionTime = millis();
      pirState = 1;
    } else {
      pirState = 0; // Debounce false trigger
    }
  } else if (pirState == LOW && lastPirState == HIGH) {
    pirState = 0;
  }
  lastPirState = pirState;
  
  // 3. Send HTTP POST to Flask Server
  if (WiFi.status() == WL_CONNECTED && server_ip != "") {
    HTTPClient http;
    String url = "http://" + server_ip + ":" + String(server_port) + "/api/data";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON Payload
    String payload = "{\"distance_cm\":" + String(distance_cm) + ",\"pir_state\":" + String(pirState) + "}";
    
    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode > 0) {
      Serial.print("Sent: "); Serial.print(payload);
      Serial.print(" -> Server responded: "); Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending POST request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.reconnect();
  }
  
  delay(1000); // Send data every 1 second
}
