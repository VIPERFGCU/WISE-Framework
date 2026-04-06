import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime, timezone

# --- CONFIG ---
# CLOUD_BROKER_HOST = "34.70.15.187" 

# Dev Env IP.
CLOUD_BROKER_HOST = "10.42.0.59"
CLOUD_BROKER_PORT = 1883

# Backend Topic Schema
TOPIC_DATA   = "devices/{}/data"
TOPIC_STATUS = "devices/{}/status"
TOPIC_CONTROL_SUB = "devices/+/control"

# Local Mesh Settings
LOCAL_BROKER_HOST = "10.42.0.1" # Match MESH Hotspot.
LOCAL_BROKER_PORT = 1883
LOCAL_MESH_IN_TOPIC  = "mesh/#" 
LOCAL_OTA_CMD_TOPIC  = "mesh/ota/command"
LOCAL_MESH_OUT_TOPIC = "mesh/{}/command"

cloud_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "Pi_Gateway_Cloud")
local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "Pi_Gateway_Local")

device_id_cnt = 0   # A straight counter for every new device
mac_to_id_map = {}  # Dictionary to store MAC -> Assigned ID mappings

upload_timing = {}

def on_local_connect(client, userdata, flags, rc, props):
    print(f"[LOCAL] Connected to broker with result code: {rc}")
    client.subscribe(LOCAL_MESH_IN_TOPIC)
    print(f"[LOCAL] Subscribed to {LOCAL_MESH_IN_TOPIC}")
    
def on_local_message(client, userdata, msg):
    """
    Received packet from Mesh.
    Format: {"id": "esp32_XXXX", "type": "data", "t_start": 123456, "vals": [[x,y,z], ...]}
    """
    global device_id_cnt 
    
    try:
        raw = json.loads(msg.payload.decode())
        device_id = raw.get("id", "unknown") # Expected to be MAC address during assignment
        msg_type = raw.get("type", "data")
        
        # Packet Handling
        if msg_type == "data":
            # Extract Batch Info
            vals = raw.get("vals", [])
            base_ts_us = int(raw.get("t_start", 0)) * 1000 
            interval_us = int(raw.get("interval", 0)) * 1_000_000

            # --- TIMING ADJUSTMENT LOGIC ---
            if device_id in upload_timing:
                expected_base_ts = upload_timing[device_id]
                
                # Check the difference between reported time and expected time
                drift = abs(base_ts_us - expected_base_ts)
                
                # If the drift is within 3 intervals, it's just network/processing jitter. 
                # Snap it to the expected timestamp to maintain perfect continuity.
                if drift < (interval_us * 3):
                    base_ts_us = expected_base_ts
                else:
                    print(f"[WARN] {device_id}: Large time gap/drift detected. Resetting baseline.")
            
            # Update expected next timestamp for the next batch from this device
            upload_timing[device_id] = base_ts_us + (len(vals) * interval_us)

            # --- FORWARD BATCH TO CLOUD MQTT ---
            # Repackage the data with the mathematically corrected timestamp
            out_payload = {
                "device_id": device_id,
                "type": "data_batch",
                "t_start_us": base_ts_us,
                "interval_us": interval_us,
                "vals": vals
            }

            if len(vals) > 0:
                cloud_client.publish(TOPIC_DATA.format(device_id), json.dumps(out_payload))
                print(f"[DATA] Forwarded batch of {len(vals)} records for {device_id} to Cloud")

        elif msg_type == "client_assignment":
            # First request from every newly connected client device
            if device_id in mac_to_id_map:
                # We already know this device, fetch its existing ID
                assigned_id = mac_to_id_map[device_id]
                print(f"[REGISTER] Device MAC {device_id} reconnected. Sending existing ID {assigned_id}")
            else:
                # Brand new device, increment counter and save to map
                device_id_cnt += 1
                assigned_id = device_id_cnt
                mac_to_id_map[device_id] = assigned_id
                print(f"[REGISTER] New device registered. MAC: {device_id} -> ID: {assigned_id}")

            out = {
                "device_id": device_id,
                "type" : "set_id",
                "payload" : str(assigned_id)
            }

            # Publishing to Mesh and Cloud
            local_client.publish(LOCAL_MESH_OUT_TOPIC.format(device_id), json.dumps(out))
            cloud_client.publish(TOPIC_DATA.format(device_id), json.dumps(out))
            print(f"[REGISTER] Registration forwarded for {device_id}")

        # HEARTBEAT Packet Handling
        elif msg_type == "heartbeat":
            out = {
                "id": device_id,
                "state": "online",
                "ts": datetime.now(timezone.utc).isoformat(),
                "meta": raw.get("payload", {}) 
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
    try:
        cmd = json.loads(msg.payload.decode())
        if cmd.get("cmd") == "OTA":
            ota_url = cmd.get("url")
            
            if not ota_url:
                target_ip = cmd.get("ip", "10.42.0.1") 
                filename = cmd.get("file", "update.bin")
                ota_url = f"http://{target_ip}:8000/{filename}"
            
            local_client.publish(LOCAL_OTA_CMD_TOPIC, ota_url)
            print(f"[OTA] Triggered Root Update: {ota_url}")
            
    except Exception as e:
        print(f"[ERROR] Failed to process cloud command: {e}")
    except:
        pass

# Setup & Loop
local_client.on_connect = on_local_connect
local_client.on_message = on_local_message
local_client.connect(LOCAL_BROKER_HOST, LOCAL_BROKER_PORT, 60)
# Sub removed from here to prevent reboot amnesia
local_client.loop_start()

cloud_client.on_connect = on_cloud_connect
cloud_client.on_message = on_cloud_message
cloud_client.connect(CLOUD_BROKER_HOST, CLOUD_BROKER_PORT, 60)
cloud_client.loop_forever()