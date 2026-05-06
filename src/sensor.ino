/*
 * ============================================================
 * SMART DOOR DETECTION SYSTEM (Wi-Fi Enterprise Version)
 * ============================================================
 * 
 * ESP32 Code untuk membaca:
 *   1. Sensor Ultrasonik HC-SR04 (deteksi pintu terbuka/tertutup)
 *   2. Sensor PIR HC-SR501 (deteksi keberadaan manusia)
 * 
 * Data dikirim via Wi-Fi (WPA2-Enterprise) ke Server SSH (Python)
 * Format: DATA:jarak_cm,status_pir
 * 
 * Pin Connections:
 *   ESP32                    Sensor Ultrasonik HC-SR04
 *   -----                    -------------------------
 *   5V    -----------------> VCC
 *   GND   -----------------> GND
 *   GPIO 5 -----------------> TRIG
 *   GPIO 18 -----------------> ECHO
 * 
 *   ESP32                    Sensor PIR HC-SR501
 *   -----                    -----------------
 *   5V    -----------------> VCC
 *   GND   -----------------> GND
 *   GPIO 19 -----------------> OUT
 * 
 * ============================================================
 */

#include <WiFi.h>
#include "esp_wpa2.h"

// =============== DEFINE PIN ===============
// Pin untuk Sensor Ultrasonik
#define TRIG_PIN 5      // Pin trigger (kirim sinyal ultrasonic)
#define ECHO_PIN 18     // Pin echo (terima sinyal pantul)

// Pin untuk Sensor PIR
#define PIR_PIN 19      // Pin output PIR (HIGH = ada orang, LOW = tidak ada)

// =============== KONFIGURASI JARINGAN ===============
#include "secrets.h"

// Konfigurasi Server
const char* server_ip = "10.6.6.41";
const int server_port = 5003;

WiFiClient client;

// =============== VARIABEL GLOBAL ===============
long duration;          
float distance_cm;      
int pirState = 0;       
int lastPirState = 0;   
unsigned long lastMotionTime = 0;     
const unsigned long motionDebounce = 2000;  

// Timer untuk reconnect
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000;

// =============== SETUP ===============
void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);    
  pinMode(ECHO_PIN, INPUT);     
  pinMode(PIR_PIN, INPUT);      
  
  Serial.println("\n=========================================");
  Serial.println("SMART DOOR DETECTION SYSTEM (Wi-Fi)");
  Serial.println("=========================================");
  
  // Koneksi ke Wi-Fi WPA2-Enterprise
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();
  
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Waiting for PIR sensor to stabilize (30s)...");
  delay(30000);  
  
  Serial.println("System READY! Starting monitoring...");
  Serial.println("=========================================");
}

// =============== FUNGSI RECONNECT SERVER ===============
void checkServerConnection() {
  if (!client.connected()) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastReconnectAttempt >= reconnectInterval) {
      lastReconnectAttempt = currentMillis;
      Serial.print("Attempting to connect to server ");
      Serial.print(server_ip);
      Serial.print(":");
      Serial.println(server_port);
      
      if (client.connect(server_ip, server_port)) {
        Serial.println("Connected to server!");
      } else {
        Serial.println("Connection failed. Retrying later...");
      }
    }
  }
}

// =============== LOOP ===============
void loop() {
  // Pastikan koneksi Wi-Fi tetap aktif
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Waiting for reconnect...");
    delay(1000);
    return;
  }

  // Cek dan hubungkan ulang ke server jika terputus
  checkServerConnection();
  
  // Membaca Sensor Ultrasonik
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  if (duration == 0) {
    distance_cm = 999;  
  } else {
    distance_cm = duration * 0.034 / 2;
  }
  
  // Membaca Sensor PIR
  pirState = digitalRead(PIR_PIN);
  
  if (pirState == HIGH && lastPirState == LOW) {
    if (millis() - lastMotionTime > motionDebounce) {
      lastMotionTime = millis();  
      pirState = 1;               
    } else {
      pirState = 0;
    }
  }
  else if (pirState == LOW && lastPirState == HIGH) {
    pirState = 0;  
  }
  
  lastPirState = pirState;
  
  // Format Data
  String dataString = "DATA:" + String(distance_cm) + "," + String(pirState);
  
  // Kirim data via Serial untuk debugging (USB)
  Serial.println(dataString);
  
  // Kirim data via Wi-Fi ke Server
  if (client.connected()) {
    client.println(dataString);
  }
  
  delay(500);
}

// ============================================================
// PENJELASAN TAMBAHAN
// ============================================================
/*
 * INTERPRETASI DATA DI SERVER:
 * 
 * Data dari ESP32 -> | Makna di Server
 * -------------------|-------------------------------------------
 * DATA:5.2,0        | Pintu TERTUTUP (jarak 5cm), TIDAK ADA orang
 * DATA:5.2,1        | Pintu TERTUTUP (jarak 5cm), ADA orang lewat
 * DATA:45.3,0       | Pintu TERBUKA (jarak 45cm), TIDAK ADA orang
 * DATA:45.3,1       | Pintu TERBUKA (jarak 45cm), ADA orang masuk
 *
 * AMBANG BATAS PINTU (bisa diatur di server):
 * - Jarak < 15 cm  = Pintu TERTUTUP
 * - Jarak >= 15 cm = Pintu TERBUKA
 */
