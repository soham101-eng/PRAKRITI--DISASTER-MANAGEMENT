import time
import random
import requests

API_URL = "http://localhost:8000/api/telemetry"

print("[SIMULATOR] Starting live hardware telemetry stream...")
print("[SIMULATOR] Base Water Level set to ~0.00m with physical jitter.")

seq = 1

while True:
    try:
        # Water level around 0.00m with realistic ultrasonic sensor noise (0.00 to 0.04m)
        noise = random.choice([0.00, 0.01, 0.02, 0.03, 0.00, 0.04])
        water_level = round(max(0.00, noise), 2)

        # Fluctuating ambient conditions
        rainfall = round(max(0.0, random.gauss(0.0, 1.2)), 2)  # Calm / slight mist
        temperature = round(random.uniform(30.8, 32.2), 1)     # Hooghly / Kolkata midday
        pressure = round(random.uniform(1003.5, 1006.2), 1)    # Ambient pressure jitter
        humidity = round(random.uniform(74.0, 81.0), 1)        # Tropical humidity
        soil_moisture = round(random.uniform(22.0, 28.0), 1)

        # Dynamic risk calculation based on baseline
        if water_level > 1.8 or rainfall > 50.0:
            risk_state = "CRITICAL"
            fused_score = random.uniform(80.0, 95.0)
        elif water_level > 0.8 or rainfall > 20.0:
            risk_state = "WARNING"
            fused_score = random.uniform(50.0, 75.0)
        elif water_level > 0.3 or rainfall > 5.0:
            risk_state = "WATCH"
            fused_score = random.uniform(25.0, 45.0)
        else:
            risk_state = "NORMAL"
            fused_score = round(random.uniform(8.0, 16.0), 2)

        payload = {
            "node_id": 1,
            "seq": seq,
            "water_level": water_level,
            "rainfall": rainfall,
            "pressure": pressure,
            "temperature": temperature,
            "humidity": humidity,
            "soil_moisture": soil_moisture,
            "fused_score": fused_score,
            "risk_state": risk_state,
            "rssi": round(random.uniform(-75.0, -82.0), 1)
        }

        # Push to FastAPI backend
        res = requests.post(API_URL, json=payload, timeout=1.5)
        print(f"[PACKET #{seq}] Water: {water_level}m | Rain: {rainfall}mm/h | State: {risk_state} -> Status: {res.status_code}")
        
        seq += 1

    except Exception as err:
        print(f"[SIMULATOR ERROR] Backend unreachable: {err}")

    # Transmit every 2 seconds to match LoRa sampling rate
    time.sleep(2)