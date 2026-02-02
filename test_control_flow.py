#!/usr/bin/env python3
"""
Test the full control flow: Frontend -> API -> MQTT -> Device
"""
import os
import sys
import json
import time
import paho.mqtt.client as mqtt
import subprocess
from threading import Thread, Event

# Config
MQTT_HOST = os.getenv("MQTT_HOST", "10.0.0.155")  # Real broker
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
API_URL = "http://localhost:8000"
DEVICE_ID = "bridge-esp32-001"

# Track messages received
messages_received = []
received_event = Event()

def on_connect(client, userdata, flags, rc, properties=None):
    print(f"[Listener] Connected with code {rc}")
    client.subscribe("devices/#")

def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    print(f"[Listener] {msg.topic}: {payload}")
    messages_received.append((msg.topic, payload))
    received_event.set()

# Start listener thread
print("=" * 60)
print("MQTT Control Flow Diagnostic")
print("=" * 60)
print(f"MQTT Broker: {MQTT_HOST}:{MQTT_PORT}")
print(f"API URL: {API_URL}")
print(f"Device ID: {DEVICE_ID}")
print()

listener = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
listener.on_connect = on_connect
listener.on_message = on_message

try:
    listener.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    listener.loop_start()
    print("[✓] Listener connected to MQTT broker")
except Exception as e:
    print(f"[✗] Failed to connect listener: {e}")
    sys.exit(1)

# Give listener time to subscribe
time.sleep(1)

# Test 1: Call the START endpoint
print("\n" + "=" * 60)
print("Test 1: POST /api/v1/devices/{id}/start")
print("=" * 60)
messages_received.clear()
received_event.clear()

try:
    result = subprocess.run([
        "curl", "-X", "POST", f"{API_URL}/api/v1/devices/{DEVICE_ID}/start",
        "-H", "Content-Type: application/json", "-d", '{"rate_hz": 10}'
    ], capture_output=True, timeout=5, text=True)
    print(f"[✓] API returned: {result.stdout}")
except Exception as e:
    print(f"[✗] API call failed: {e}")

# Wait for MQTT message
print("Waiting for MQTT messages (5s)...")
for i in range(5):
    if messages_received:
        print(f"[✓] Received {len(messages_received)} message(s):")
        for topic, payload in messages_received:
            print(f"    {topic}: {payload}")
        break
    time.sleep(1)
else:
    print("[✗] No MQTT messages received")

# Test 2: Check device status topic
print("\n" + "=" * 60)
print("Test 2: Checking for device status and data")
print("=" * 60)
messages_received.clear()
print("Listening for 3 seconds...")
time.sleep(3)

if messages_received:
    print(f"[✓] Received {len(messages_received)} message(s):")
    for topic, payload in messages_received:
        print(f"    {topic}: {payload}")
else:
    print("[✗] No messages received")

# Test 3: Call STOP
print("\n" + "=" * 60)
print("Test 3: POST /api/v1/devices/{id}/stop")
print("=" * 60)
messages_received.clear()

try:
    result = subprocess.run([
        "curl", "-X", "POST", f"{API_URL}/api/v1/devices/{DEVICE_ID}/stop",
        "-H", "Content-Type: application/json"
    ], capture_output=True, timeout=5, text=True)
    print(f"[✓] API returned: {result.stdout}")
except Exception as e:
    print(f"[✗] API call failed: {e}")

print("Waiting for MQTT messages (3s)...")
time.sleep(3)
if messages_received:
    print(f"[✓] Received {len(messages_received)} message(s):")
    for topic, payload in messages_received:
        print(f"    {topic}: {payload}")
else:
    print("[✗] No MQTT messages received")

# Cleanup
listener.loop_stop()
listener.disconnect()
print("\n[✓] Test complete")
