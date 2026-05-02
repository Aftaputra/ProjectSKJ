# Smart Door Detection System

Sistem monitoring pintu dan deteksi orang berbasis client-server TCP/IP. ESP32 membaca sensor ultrasonik (deteksi pintu terbuka/tertutup) dan sensor PIR (deteksi manusia), mengirim data ke server di Jetson Nano, kemudian client di laptop dapat memantau status secara real-time.

## Deskripsi

- ESP32 membaca sensor ultrasonik (jarak pintu) dan sensor PIR (ada/tidak orang)
- Data dikirim ke server (Jetson Nano) melalui kabel serial USB
- Server memproses data dan menyediakan API untuk client
- Client (laptop) terhubung ke server via TCP/IP
- Client bisa meminta status atau menerima notifikasi real-time

## Tujuan

- Memonitor status pintu (terbuka/tertutup) secara real-time
- Mendeteksi keberadaan orang di area pintu
- Memberikan notifikasi otomatis ke client yang terhubung
- Client dapat memeriksa status kapan saja via command

## Fitur

- Deteksi pintu terbuka/tertutup (sensor ultrasonik HC-SR04)
- Deteksi keberadaan orang (sensor PIR HC-SR501)
- Notifikasi real-time dari server ke client
- Client dapat request status (command `status`)
- Client dapat monitoring berkelanjutan (command `stream`)
- Support multiple client koneksi
- Auto-notifikasi saat pintu berubah status atau ada orang

## Hardware Requirements

| Komponen | Fungsi |
|----------|--------|
| ESP32 | Mikrokontroler pembaca sensor |
| Sensor Ultrasonik HC-SR04 | Deteksi jarak pintu |
| Sensor PIR HC-SR501 | Deteksi keberadaan manusia |
| Jetson Nano | Menjalankan server Python |
| Laptop | Menjalankan client Python |
| Kabel jumper & breadboard | Rangkaian |

## Software Requirements

| Perangkat | Software |
|-----------|----------|
| ESP32 | Arduino IDE / PlatformIO |
| Jetson Nano | Python 3 + pyserial |
| Laptop | Python 3 |

## Skema Rangkaian (ESP32 ke Sensor)

| ESP32 | Sensor Ultrasonik HC-SR04 |
|-------|---------------------------|
| 5V | VCC |
| GND | GND |
| GPIO 5 | TRIG |
| GPIO 18 | ECHO |

| ESP32 | Sensor PIR HC-SR501 |
|-------|---------------------|
| 5V | VCC |
| GND | GND |
| GPIO 19 | OUT |

## Instalasi

### 1. ESP32
Upload kode `esp32_sensor.ino` ke ESP32 menggunakan Arduino IDE

### 2. Jetson Nano (Server)
```bash
# Install pyserial
pip3 install pyserial

# Cek port USB ESP32
ls /dev/ttyUSB*

# Jalankan server
python3 server.py
