import json
import sqlite3
from flask import Flask, request, jsonify
from datetime import datetime

# Load configuration
try:
    with open('config.json', 'r') as f:
        config = json.load(f)
except FileNotFoundError:
    print("config.json not found, using defaults.")
    config = {"host": "0.0.0.0", "port": 5003, "database": "package_deliveries.db", "package_height_threshold_cm": 8.0}

app = Flask(__name__)
DB_FILE = config.get('database', 'package_deliveries.db')
THRESHOLD = config.get('package_height_threshold_cm', 8.0)

# Stateful variables for Dynamic Baseline Tracking
current_baseline = 0.0
saved_baseline = 0.0
is_motion_active = False
settling_ticks = 0

def init_db():
    conn = sqlite3.connect(DB_FILE)
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS package_deliveries (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            old_baseline_cm REAL,
            new_baseline_cm REAL,
            package_height_cm REAL
        )
    ''')
    conn.commit()
    conn.close()

@app.route('/api/data', methods=['POST'])
def receive_data():
    global current_baseline, saved_baseline, is_motion_active, settling_ticks
    
    try:
        data = request.get_json()
        if not data:
            return jsonify({"status": "error", "message": "No JSON provided"}), 400
            
        distance = data.get('distance_cm')
        pir = data.get('pir_state')
        
        if distance is None or pir is None or distance == 999:
            return jsonify({"status": "error", "message": "Invalid fields"}), 400
            
        if pir == 1:
            if not is_motion_active:
                # MOTION JUST STARTED!
                is_motion_active = True
                saved_baseline = current_baseline
                settling_ticks = 0
                print(f"[{datetime.now().strftime('%H:%M:%S')}] ✋ Motion Detected. Saved baseline: {saved_baseline:.1f}cm. Waiting for hand to leave...")
            else:
                # Still in motion. Reset settling ticks just in case the sensor flickered.
                settling_ticks = 0
                
        elif pir == 0:
            if is_motion_active:
                # Motion ended, but we wait for 3 consecutive ticks (3 seconds) to ensure the hand 
                # is completely gone and the ultrasonic sensor has a clean reading of the package.
                settling_ticks += 1
                if settling_ticks >= 3:
                    is_motion_active = False
                    new_distance = distance
                    
                    # Calculate the difference
                    difference = saved_baseline - new_distance
                    
                    if difference >= THRESHOLD:
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] 📦 PACKAGE DELIVERED! Height: {difference:.1f}cm (Base: {saved_baseline:.1f}cm -> {new_distance:.1f}cm)")
                        conn = sqlite3.connect(DB_FILE)
                        c = conn.cursor()
                        c.execute("INSERT INTO package_deliveries (old_baseline_cm, new_baseline_cm, package_height_cm) VALUES (?, ?, ?)", 
                                  (saved_baseline, new_distance, difference))
                        conn.commit()
                        conn.close()
                    elif difference <= -THRESHOLD:
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] 📬 MAILBOX EMPTIED! (Base: {saved_baseline:.1f}cm -> {new_distance:.1f}cm)")
                    else:
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] 👻 FALSE ALARM (Courier looked inside but left nothing).")
                        
                    # Hard-reset the baseline to the new reality
                    current_baseline = new_distance
                    settling_ticks = 0
            else:
                # Idle state: continuously update the baseline
                if current_baseline == 0.0:
                    current_baseline = distance
                else:
                    # Exponential Moving Average
                    current_baseline = (current_baseline * 0.8) + (distance * 0.2)

        return jsonify({"status": "success"}), 200
        
    except Exception as e:
        print("Error:", str(e))
        return jsonify({"status": "error", "message": str(e)}), 500

@app.route('/api/deliveries', methods=['GET'])
def get_deliveries():
    limit = request.args.get('limit', 50)
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    c = conn.cursor()
    c.execute("SELECT * FROM package_deliveries ORDER BY timestamp DESC LIMIT ?", (limit,))
    rows = c.fetchall()
    conn.close()
    
    result = [dict(row) for row in rows]
    return jsonify({"status": "success", "data": result}), 200

@app.route('/api/deliveries', methods=['DELETE'])
def clear_deliveries():
    try:
        conn = sqlite3.connect(DB_FILE)
        c = conn.cursor()
        c.execute("DELETE FROM package_deliveries")
        conn.commit()
        conn.close()
        return jsonify({"status": "success", "message": "Delivery log cleared."}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == '__main__':
    init_db()
    print(f"Starting Package Delivery Server on {config.get('host')}:{config.get('port')}")
    app.run(host=config.get('host'), port=config.get('port'), debug=False)
