import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime, timezone

# --- CONFIG ---
CLOUD_BROKER_HOST = "192.168.1.79" 
CLOUD_BROKER_PORT = 1883

# Backend Topic Schema
TOPIC_DATA   = "devices/{}/data"
TOPIC_STATUS = "devices/{}/status"
TOPIC_CONTROL_SUB = "devices/+/control"

# Local Mesh Settings
LOCAL_BROKER_HOST = "localhost"
LOCAL_BROKER_PORT = 1883
LOCAL_MESH_IN_TOPIC  = "mesh/#" # Listen for everything from mesh --- Do I need to refine this?
LOCAL_OTA_CMD_TOPIC  = "mesh/ota/command"

cloud_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "Pi_Gateway_Cloud")
local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "Pi_Gateway_Local")

def on_local_message(client, userdata, msg):
    """
    Received packet from Mesh.
    Format: {"id": "esp32_XXXX", "type": "data|heartbeat", "payload": {...}}
    """
    try:
        raw = json.loads(msg.payload.decode())
        device_id = raw.get("id", "unknown")
        msg_type = raw.get("type", "data")
        
        # Packet Handling
        if msg_type == "data":
            # Transform x/y/z that the backend expects (subject to change -> x,y,z is template)
            vals = raw.get("val", [])
            out = {
                "device_id": device_id,
                "ts": datetime.now(timezone.utc).isoformat(),
                "x": vals[0] if len(vals) > 0 else 0.0,
                "y": vals[1] if len(vals) > 1 else 0.0,
                "z": vals[2] if len(vals) > 2 else 0.0
            }
            # Publishing to Cloud Data Topic
            cloud_client.publish(TOPIC_DATA.format(device_id), json.dumps(out))
            print(f"[DATA] Forwarded for {device_id}")

        # HEARTBEAT Packet Handling (For now plug in later)
        elif msg_type == "heartbeat":
            # Backend expects: {"id":..., "state":..., "ts":...}
            out = {
                "id": device_id,
                "state": "online",
                "ts": datetime.now(timezone.utc).isoformat(),
                "meta": raw.get("payload", {}) # Pass battery/etc as metadata?
            }
            cloud_client.publish(TOPIC_STATUS.format(device_id), json.dumps(out))
            print(f"[HEARTBEAT] Forwarded for {device_id}")

    except Exception as e:
        print(f"Error: {e}")

def on_cloud_connect(client, userdata, flags, rc, props):
    print(f"Cloud Connected: {rc}")
    client.subscribe(TOPIC_CONTROL_SUB)

def on_cloud_message(client, userdata, msg):
    """
    CONTROL commands from Cloud (e.g. OTA Start)
    """
    print(f"[CONTROL] Cmd received on {msg.topic}")
    # Check if payload contains OTA command
    try:
        cmd = json.loads(msg.payload.decode())
           # Check for OTA Command
        if cmd.get("cmd") == "OTA":
            # 1. Try to get the full URL from the payload
            ota_url = cmd.get("url")
            
            # 2. If no URL, build a default one (Safe Fallback)
            if not ota_url:
                target_ip = cmd.get("ip", "10.10.10.1") # Default to Mesh Gateway IP
                filename = cmd.get("file", "update.bin")
                ota_url = f"http://{target_ip}:8000/{filename}"
            
            # 3. Publish to Mesh
            local_client.publish(LOCAL_OTA_CMD_TOPIC, ota_url)
            print(f"[OTA] Triggered Root Update: {ota_url}")
            
    except Exception as e:
        print(f"[ERROR] Failed to process cloud command: {e}")
    except:
        pass

# Setup & Loop
local_client.on_message = on_local_message
local_client.connect(LOCAL_BROKER_HOST, LOCAL_BROKER_PORT, 60)
local_client.subscribe(LOCAL_MESH_IN_TOPIC)
local_client.loop_start()

cloud_client.on_connect = on_cloud_connect
cloud_client.on_message = on_cloud_message
cloud_client.connect(CLOUD_BROKER_HOST, CLOUD_BROKER_PORT, 60)
cloud_client.loop_forever()