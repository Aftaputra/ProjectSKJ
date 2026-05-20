import json
import requests
import time
import threading
import sys

# Global state to track the latest delivery ID
last_seen_id = None

def load_config():
    try:
        with open('config.json', 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        return {"host": "127.0.0.1", "port": 5003}

def get_latest_delivery_id(url):
    try:
        response = requests.get(url, params={"limit": 1})
        if response.status_code == 200:
            data = response.json().get('data', [])
            if data:
                return data[0]['id']
    except requests.exceptions.ConnectionError:
        pass
    return None

def background_monitor():
    global last_seen_id
    config = load_config()
    url = f"http://127.0.0.1:{config.get('port', 5003)}/api/deliveries"
    
    # Initialize the baseline ID so we don't alert for old packages on startup
    last_seen_id = get_latest_delivery_id(url)
    
    while True:
        time.sleep(3) # Check for new deliveries every 3 seconds
        try:
            response = requests.get(url, params={"limit": 1})
            if response.status_code == 200:
                data = response.json().get('data', [])
                if data:
                    latest = data[0]
                    # If the latest ID in the database is higher than the last one we saw
                    if last_seen_id is not None and latest['id'] > last_seen_id:
                        last_seen_id = latest['id']
                        
                        # We use \r to overwrite the current "Select an option: " input prompt
                        # Then we print the alert (with \a for a system bell sound)
                        sys.stdout.write("\r" + " " * 50 + "\r") 
                        print(f"\n🚨 📦 DING DONG! NEW PACKAGE DELIVERED! 📦 🚨")
                        print(f"[{latest['timestamp']}] Height: {latest['package_height_cm']:.1f} cm")
                        print("-------------------------------------------\n")
                        
                        # Redraw the prompt so the user knows they can still type
                        sys.stdout.write("Select an option: ")
                        sys.stdout.flush()
                        
                    elif last_seen_id is None:
                        last_seen_id = latest['id']
                        
        except requests.exceptions.ConnectionError:
            pass # Silently ignore connection errors in the background

def fetch_deliveries(limit=10):
    config = load_config()
    url = f"http://127.0.0.1:{config.get('port', 5003)}/api/deliveries"
    
    try:
        response = requests.get(url, params={"limit": limit})
        if response.status_code == 200:
            data = response.json().get('data', [])
            print(f"\n--- 📦 Last {len(data)} Package Deliveries 📦 ---")
            if not data:
                print("No deliveries logged yet.")
            for entry in data:
                print(f"[{entry['timestamp']}] Package Height: {entry['package_height_cm']:.1f} cm (Baseline changed: {entry['old_baseline_cm']:.1f} -> {entry['new_baseline_cm']:.1f})")
            print("-------------------------------------------\n")
        else:
            print(f"Error fetching data: {response.text}")
    except requests.exceptions.ConnectionError:
        print("Could not connect to the server. Make sure server.py is running.")

def clear_logs():
    global last_seen_id
    config = load_config()
    url = f"http://127.0.0.1:{config.get('port', 5003)}/api/deliveries"
    
    confirm = input("Are you sure you want to clear all delivery logs? (y/n): ")
    if confirm.lower() != 'y':
        print("Cancelled.")
        return
        
    try:
        response = requests.delete(url)
        if response.status_code == 200:
            print("✅ Delivery log successfully cleared!")
            last_seen_id = None # Reset the monitor tracker
        else:
            print(f"Error clearing logs: {response.text}")
    except requests.exceptions.ConnectionError:
        print("Could not connect to the server. Make sure server.py is running.")

def print_menu():
    print("\n📬 Smart Drop-Box Dashboard")
    print("1. View recent deliveries")
    print("2. Clear delivery log")
    print("3. Exit")

if __name__ == "__main__":
    # Start the background polling thread
    monitor_thread = threading.Thread(target=background_monitor, daemon=True)
    monitor_thread.start()
    
    print_menu()
    while True:
        choice = input("Select an option: ")
        
        if choice == '1':
            fetch_deliveries(10)
            print_menu()
        elif choice == '2':
            clear_logs()
            print_menu()
        elif choice == '3':
            break
        elif choice != '': # Ignore empty enters
            print("Invalid option.")
            print_menu()
