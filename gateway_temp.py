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
CLOUD_BROKER_HOST = "wise-net.io" 
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
LOCAL_MESH_OUT_TOPIC = "mesh/{}/command"

cloud_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "Pi_Gateway_Cloud")
local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "Pi_Gateway_Local")

device_id_cnt = 0   # A straight counter for every new device

def on_local_message(client, userdata, msg):
    """
    Received packet from Mesh.
    Format: {"id": "esp32_XXXX", "type": "data", "t_start": 123456, "vals": [[x,y,z], ...]}
    """
    global device_id_cnt 
    
    try:
        raw = json.loads(msg.payload.decode())
        device_id = raw.get("id", "unknown")
        msg_type = raw.get("type", "data")
        
        # Packet Handling
        if msg_type == "data":
            # Extract Batch Info
            vals = raw.get("vals", [])

            base_ts_us = int(raw.get("t_start", 0)) * 1000 
            interval_us = int(raw.get("interval", 0)) * 1_000_000

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
            # Bulk write to InfluxDB
            if len(points_buffer) > 1:
                write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=points_buffer)
                print(f"[DATA] Wrote {len(points_buffer)} records for {device_id}")


        elif msg_type == "client_assignment":
            # First request from every newly connected client device
            # By default the device id will be the device's MAC address
            device_id_cnt += 1
            new_device_id = device_id_cnt
            out = {
                "device_id": device_id,
                "type" : "set_id",
                "payload" : str(new_device_id)
            }
            
            server_out = {
                "device_id": new_device_id,
                "type" : "client_assignment",
                # "payload" : str(new_device_id)
            }

            # Publishing to Cloud Data Topic
            local_client.publish(LOCAL_MESH_OUT_TOPIC.format(device_id), json.dumps(out))
            cloud_client.publish("mesh/", json.dumps(server_out))
            print(f"New device registered {new_device_id}")

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
        print(f"Error processing message: {e}")

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
        if cmd.get("cmd") == "OTA":
            # Command Root Node to start OTA
            # URL of file on pi
            ota_url = f"http://{cmd.get('ip', '10.0.0.194')}:8000/fgcu-esp32.bin"
            local_client.publish(LOCAL_OTA_CMD_TOPIC, ota_url)
            print(f"[OTA] Triggered Root Update: {ota_url}")
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