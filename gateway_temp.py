import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime, timezone
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import ASYNCHRONOUS

# --- INFLUXDB CONFIG ---
INFLUX_URL = "wise-net.io:8086"
INFLUX_TOKEN = "super-long-admin-token"  # Replace with your actual token
INFLUX_ORG = "my-org"
INFLUX_BUCKET = "sensors"

# Initialize Influx Client
influx_client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = influx_client.write_api(write_options=ASYNCHRONOUS)

# --- CONFIG ---
# CLOUD_BROKER_HOST = "34.70.15.187" 

# Dev Env IP.
CLOUD_BROKER_HOST = "10.42.0.59"

# CLOUD_BROKER_HOST = "wise-net.io" 
CLOUD_BROKER_PORT = 1883

# Backend Topic Schema
TOPIC_DATA   = "devices/{}/data"
TOPIC_STATUS = "devices/{}/status"
TOPIC_CONTROL_SUB = "devices/+/control"

# Local Mesh Settings
LOCAL_BROKER_HOST = "10.42.0.1" #Match MESH Hotsport.
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
            
            # -------------------------------

            points_buffer = []

            for i, record in enumerate(vals):
                if len(record) < 3: continue
                
                current_ts = base_ts_us + (i * interval_us)

                p = Point("accelerometer") \
                    .tag("device_id", device_id) \
                    .field("x", float(record[0])) \
                    .field("y", float(record[1])) \
                    .field("z", float(record[2])) \
                    .time(current_ts, WritePrecision.NS)
                
                points_buffer.append(p)
                                        
            # Update expected next timestamp for the next batch from this device
            upload_timing[device_id] = base_ts_us + (len(vals) * interval_us)

            # Bulk write to InfluxDB
            if len(points_buffer) > 0:
                write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=points_buffer)
                print(f"[DATA] Wrote {len(points_buffer)} records for {device_id}")

        elif msg_type == "client_assignment":
            # First request from every newly connected client device
            # device_id is expected to be the device's MAC address here
            
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
            
            server_out = {
                "device_id": assigned_id,
                "type" : "client_assignment",
                # "payload" : str(assigned_id)
            }

            # Publishing to Cloud Data Topic
            local_client.publish(LOCAL_MESH_OUT_TOPIC.format(device_id), json.dumps(out))
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
                target_ip = cmd.get("ip", "10.42.0.1") # Default to Mesh Gateway IP
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
local_client.on_connect = on_local_connect
local_client.on_message = on_local_message
local_client.connect(LOCAL_BROKER_HOST, LOCAL_BROKER_PORT, 60)
local_client.subscribe(LOCAL_MESH_IN_TOPIC)
local_client.loop_start()

cloud_client.on_connect = on_cloud_connect
cloud_client.on_message = on_cloud_message
cloud_client.connect(CLOUD_BROKER_HOST, CLOUD_BROKER_PORT, 60)
cloud_client.loop_forever()
