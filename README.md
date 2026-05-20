# Smart Mailbox Drop-Box Detector

A highly robust, false-positive resistant package delivery detection system. The system uses an ESP32 as a sensor node to stream live hardware data over Wi-Fi to a central Python Flask server. The server tracks the state of the mailbox using a dynamic baseline algorithm to accurately log package deliveries.

## Hardware Placement Architecture

To ensure complete reliability and ignore passing cars, animals, or swinging mailbox doors, the physical layout of the sensors is critical:

### 1. The Ultrasonic Sensor (The Depth Gauge)
- **Placement**: Mounted on the inside ceiling of the box, pointing straight down at the floor.
- **Why?**: The ultrasonic sensor needs a fixed, unchanging baseline distance. If your box is 60cm deep, it constantly reads ~60cm. When a package drops in, the sensor suddenly reads a shorter distance (e.g., 40cm), proving an object is taking up physical space. Mounting it on the ceiling also prevents large packages from pressing directly against the sensor face (which creates a 2-3cm "dead zone" of garbage data).

### 2. The PIR Sensor (The Wake-Up Trigger)
- **Placement**: Mounted high on an inside wall, pointing *across* the opening (door or lid).
- **Why?**: You only want to trigger the logic when a human hand or arm reaches into the box. **Never point the PIR sensor out toward the street**, or it will falsely trigger on passing cars and pedestrians. Pointing it across the inside creates an invisible tripwire right at the threshold of the box.

---

## The Server Logic & Algorithm

The Python server (`src/server.py`) does not just log raw data; it uses a state-machine with **Dynamic Baseline Tracking** and a **Confirmation Delay** to prevent false positives.

### Dynamic Baseline Tracking
Instead of hardcoding a baseline distance (like "60cm"), the server constantly adapts:
- **Idle State (`PIR = 0`)**: The server continually updates an Exponential Moving Average of the ultrasonic distance. If temperature changes cause the speed of sound to shift, or if the sensor drifts slightly, the baseline seamlessly adapts.
- **Stacked Packages**: If one package is already in the box, the server adapts its new baseline to the top of that package. If a second package is dropped, it accurately detects the height difference between the first and second package!

### The Delivery State Machine
1. **Motion Starts (`PIR = 1`)**: A courier's hand reaches in. The server immediately freezes the current dynamic baseline (e.g., 60cm) and waits.
2. **Motion Ends (`PIR = 0`)**: The hand leaves the box. The PIR sensor has a hardware delay, meaning the hand has been gone for several seconds before the signal drops to 0.
3. **Confirmation Delay**: To ensure the hand isn't accidentally caught by the ultrasonic sensor as it pulls away, the server waits for **3 consecutive seconds** of no motion.
4. **The Check**: After the 3-second settling time, the server takes the new, clean ultrasonic distance and compares it to the frozen baseline.
   - If `Baseline - New Distance >= Threshold` (e.g., 8cm), a **Package is Delivered**! The package height is calculated and logged to the SQLite database.
   - If `Baseline - New Distance <= -Threshold`, a package was removed (**Mailbox Emptied**).
   - If the difference is negligible, it registers a **False Alarm** (e.g., someone looked inside but left nothing).

---

## Components & Setup Instructions

- **`src/sensor/sensor.ino`**: The ESP32 hardware code. Features a **Captive Portal** for configuring Wi-Fi (supports Standard WPA2 and WPA2-Enterprise) and setting the Python Server IP, all without ever needing to recompile the code. 
- **`src/server.py`**: The Flask HTTP API that receives live data, runs the tracking logic, and logs confirmed deliveries to `package_deliveries.db`.
- **`src/client.py`**: A terminal dashboard to view the delivery logs and clear them. Includes a background **Live Dashboard Mode** that instantly alerts you when a package arrives.
- **`src/config.json`**: Settings file for configuring the package threshold, ports, and database name.

### 1. Running the Server & Client
1. Install the required Python dependencies: 
   ```bash
   pip install flask requests
   ```
2. Start the server (leave this running):
   ```bash
   python src/server.py
   ```
3. Launch the client dashboard in a separate terminal:
   ```bash
   python src/client.py
   ```
   *The client will wait for your input, while a background thread continuously monitors the server. The moment a delivery arrives, it will instantly print an alert without interrupting your prompt!*

### 2. ESP32 Wi-Fi Setup
1. Flash `src/sensor/sensor.ino` to the ESP32.
2. If the ESP32 cannot connect to a known Wi-Fi network, it will broadcast an Access Point named **`ESP32-Config`**.
3. Connect your phone or laptop to this network and navigate to `http://192.168.4.1`.
4. Enter your Wi-Fi credentials and the IP address of the machine running `server.py`, then click **Save & Reboot**.
