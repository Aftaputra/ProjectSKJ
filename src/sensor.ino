/*
 * ============================================================
 * SMART DOOR DETECTION SYSTEM
 * ============================================================
 * 
 * ESP32 Code untuk membaca:
 *   1. Sensor Ultrasonik HC-SR04 (deteksi pintu terbuka/tertutup)
 *   2. Sensor PIR HC-SR501 (deteksi keberadaan manusia)
 * 
 * Data dikirim via Serial ke Jetson Nano (Server)
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

// =============== DEFINE PIN ===============
// Pin untuk Sensor Ultrasonik
#define TRIG_PIN 5      // Pin trigger (kirim sinyal ultrasonic)
#define ECHO_PIN 18     // Pin echo (terima sinyal pantul)

// Pin untuk Sensor PIR
#define PIR_PIN 19      // Pin output PIR (HIGH = ada orang, LOW = tidak ada)

// =============== VARIABEL GLOBAL ===============
// Variabel untuk Ultrasonik
long duration;          // Durasi sinyal echo (microseconds)
float distance_cm;      // Jarak hasil konversi (centimeter)

// Variabel untuk PIR
int pirState = 0;       // Status PIR saat ini (0 atau 1)
int lastPirState = 0;   // Status PIR sebelumnya (untuk deteksi perubahan)

// Variabel untuk debounce PIR (menghindari false trigger)
unsigned long lastMotionTime = 0;     // Waktu terakhir ada gerakan
const unsigned long motionDebounce = 2000;  // Cooldown 2 detik

// =============== SETUP ===============
// Fungsi ini berjalan 1 kali saat ESP32 dinyalakan
void setup() {
  
  // Inisialisasi Serial komunikasi ke Jetson Nano
  // Baud rate 115200 (kecepatan komunikasi serial)
  Serial.begin(115200);
  
  // ===== Setup Pin Mode =====
  // Ultrasonik
  pinMode(TRIG_PIN, OUTPUT);    // TRIG sebagai output (kirim sinyal)
  pinMode(ECHO_PIN, INPUT);     // ECHO sebagai input (terima sinyal)
  
  // PIR
  pinMode(PIR_PIN, INPUT);      // PIR sebagai input (baca HIGH/LOW)
  
  // ===== Pesan awal =====
  Serial.println("=========================================");
  Serial.println("SMART DOOR DETECTION SYSTEM");
  Serial.println("ESP32 Sensor Reader");
  Serial.println("=========================================");
  Serial.println("Waiting for PIR sensor to stabilize...");
  
  // PIR sensor butuh waktu 20-30 detik untuk stabil setelah dinyalakan
  // Selama waktu ini, PIR bisa mengeluarkan false trigger
  delay(30000);  // Tunggu 30 detik
  
  Serial.println("System READY! Starting monitoring...");
  Serial.println("Format data: DATA:jarak_cm,status_pir");
  Serial.println("=========================================");
}

// =============== LOOP ===============
// Fungsi ini berjalan terus menerus selama ESP32 menyala
void loop() {
  
  // ============================================================
  // BAGIAN 1: MEMBACA SENSOR ULTRASONIK (Deteksi Pintu)
  // ============================================================
  // Prinsip kerja:
  // 1. Trigger pin dikirim pulsa HIGH selama 10 microseconds
  // 2. Gelombang ultrasonic dipancarkan
  // 3. Gelombang memantul dari objek (pintu) dan kembali
  // 4. Echo pin mendeteksi gelombang pantul
  // 5. Durasi antara pancar dan terima dihitung
  // 6. Durasi dikonversi ke jarak (cm)
  // ============================================================
  
  // Step 1: Pastikan TRIG pin dalam kondisi LOW
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);  // Delay kecil untuk stabil
  
  // Step 2: Kirim pulsa HIGH selama 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Step 3: Baca durasi sinyal echo (dalam microseconds)
  // Parameter timeout 30000 microseconds = max jarak ~5 meter
  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  // Step 4: Konversi durasi ke jarak (cm)
  // Rumus: jarak = (durasi * kecepatan suara) / 2
  // Kecepatan suara = 343 m/s = 0.034 cm/microsecond
  // Dibagi 2 karena gelombang pergi-pulang
  if (duration == 0) {
    // Jika timeout (tidak ada echo) berarti terlalu jauh atau error
    distance_cm = 999;  // Nilai 999 menandakan error / tidak terdeteksi
  } else {
    distance_cm = duration * 0.034 / 2;
  }
  
  // ============================================================
  // BAGIAN 2: MEMBACA SENSOR PIR (Deteksi Manusia)
  // ============================================================
  // Prinsip kerja:
  // - Sensor PIR mendeteksi perubahan suhu (radiasi inframerah)
  // - Jika ada manusia bergerak, output menjadi HIGH (1)
  // - Jika tidak ada gerakan, output menjadi LOW (0)
  // ============================================================
  
  // Baca nilai dari sensor PIR
  pirState = digitalRead(PIR_PIN);
  
  // ===== Debounce PIR (filter untuk menghindari false trigger) =====
  // Saat PIR baru mendeteksi gerakan (LOW -> HIGH)
  if (pirState == HIGH && lastPirState == LOW) {
    // Cek apakah sudah melewati masa cooldown (2 detik)
    if (millis() - lastMotionTime > motionDebounce) {
      lastMotionTime = millis();  // Update waktu terakhir gerakan
      pirState = 1;               // Konfirmasi: ada gerakan
    } else {
      // Jika masih dalam masa cooldown, abaikan (false trigger)
      pirState = 0;
    }
  }
  // Saat gerakan berhenti (HIGH -> LOW)
  else if (pirState == LOW && lastPirState == HIGH) {
    pirState = 0;  // Tidak ada gerakan
  }
  
  // Simpan status PIR untuk perbandingan di loop berikutnya
  lastPirState = pirState;
  
  // ============================================================
  // BAGIAN 3: MENGIRIM DATA KE SERVER (Jetson Nano via Serial)
  // ============================================================
  // Format data yang dikirim: DATA:jarak_cm,status_pir
  // Contoh:
  //   DATA:25.4,0  -> Jarak 25.4cm, TIDAK ADA orang
  //   DATA:8.3,1   -> Jarak 8.3cm, ADA orang
  // ============================================================
  
  // Kirim label "DATA:"
  Serial.print("DATA:");
  
  // Kirim nilai jarak (dalam cm)
  Serial.print(distance_cm);
  
  // Kirim koma sebagai separator
  Serial.print(",");
  
  // Kirim status PIR (0 = tidak ada orang, 1 = ada orang)
  Serial.println(pirState);
  
  // ============================================================
  // BAGIAN 4: DELAY (Mengatur frekuensi pengiriman data)
  // ============================================================
  // Delay 500ms = mengirim data 2 kali per detik
  // Bisa disesuaikan:
  // - Delay lebih kecil = data lebih real-time tapi lebih berat
  // - Delay lebih besar = data lebih lambat tapi lebih ringan
  // ============================================================
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
 * 
 * NOTE: Ambang batas ini tergantung instalasi sensor.
 *       Sesuaikan dengan jarak sensor ke pintu saat tertutup.
 */

/*
 * TROUBLESHOOTING:
 * 
 * 1. Data tidak muncul di Serial Monitor?
 *    - Cek baud rate: harus 115200
 *    - Cek koneksi USB
 *    - Cek pin yang digunakan
 * 
 * 2. Jarak selalu 999?
 *    - Cek kabel TRIG dan ECHO
 *    - Cek power sensor (5V)
 *    - Coba ganti sensor
 * 
 * 3. PIR selalu 0 atau selalu 1?
 *    - Tunggu 30 detik stabilisasi
 *    - Cek koneksi OUT pin
 *    - Coba ganti sensor
 * 
 * 4. PIR terlalu sensitif?
 *    - Naikkan nilai motionDebounce (misal ke 3000)
 *    - Atur potentiometer di sensor PIR
 */
