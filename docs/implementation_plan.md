# Migrate Jetson Nano Serial Setup to Networked ESP32 (WPA2-Enterprise)

Since we are replacing the Jetson Nano (direct Serial connection) with a remote SSH server, the ESP32 must now transmit its sensor data over Wi-Fi directly to the server. Furthermore, connecting to the UGM enterprise network requires WPA2-Enterprise authentication (which uses a username/identity and password rather than just a simple Wi-Fi password).

## User Review Required

> [!IMPORTANT]
> **Network Configuration Placeholders**
> The code will include placeholders for the server IP, Wi-Fi SSID, network identity (username), and password. You will need to replace these placeholders with your actual UGM credentials and server IP before flashing the code to the ESP32.

> [!WARNING]
> **WPA2-Enterprise Library Compatibility**
> Enterprise Wi-Fi on ESP32 can occasionally be tricky depending on the specific Arduino Core version you are using. The proposed code uses the standard `esp_wpa2` library approach which works on most modern ESP32 setups.

## Open Questions

> [!NOTE]
> 1. Do you know the exact SSID for the UGM network you are targeting (e.g., `UGM-Hotspot` or `UGM-Secure`)?
> 2. Will the SSH server have a static IP address accessible from within the UGM network? The ESP32 needs a fixed IP address to send data to.

## Proposed Changes

---

### ESP32 Sensor Hardware

The Arduino sketch needs to be updated to connect to a WPA2-Enterprise network and act as a TCP client.

#### [MODIFY] [sensor.ino](file:///c:/Users/Administrator/ksjk/src/sensor.ino)
- **Add Wi-Fi Libraries:** Include `<WiFi.h>` and the ESP32 WPA2-Enterprise headers (`esp_wpa2.h`).
- **Network Configuration:** Define constants for `SSID`, `EAP_IDENTITY` (Username), `EAP_PASSWORD`, `SERVER_IP`, and `SERVER_PORT`.
- **Setup Wi-Fi:** In the `setup()` function, initialize the WPA2-Enterprise connection and wait for an IP address.
- **TCP Client Logic:** In the `loop()` function, ensure a `WiFiClient` is connected to the `SERVER_IP:SERVER_PORT`. 
- **Data Transmission:** Replace the existing `Serial.print("DATA:...");` calls with `client.print("DATA:...");` to send the data over the network instead of the serial cable (though we'll keep Serial prints for local debugging).

---

### Python Server

The Python server needs to be accessible from other devices on the network, not just locally.

#### [MODIFY] [server.py](file:///c:/Users/Administrator/ksjk/src/server.py)
- **Change Bind Address:** Change `HOST = '127.0.0.1'` to `HOST = '0.0.0.0'`. This tells the socket server to listen on all available network interfaces, allowing the ESP32 to connect to it from the network.
- **Robustness:** Ensure the server loop can handle persistent connections or client reconnections smoothly since the ESP32 might disconnect and reconnect periodically.

## Verification Plan

### Automated/Code Tests
- Ensure `server.py` runs cleanly without syntax errors and successfully binds to `0.0.0.0`.

### Manual Verification
- **ESP32 Side:** Flash `sensor.ino` to the ESP32. Open the Arduino Serial Monitor to verify that it successfully authenticates and connects to the UGM Wi-Fi, and that it attempts to connect to the server IP.
- **Server Side:** Run `server.py` on the SSH server. Ensure that the server's firewall (e.g., `ufw`) is configured to allow incoming traffic on port `5003`. Observe the server console for incoming connections and data strings from the ESP32.
