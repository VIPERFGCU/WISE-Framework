# app/mqtt.py
import paho.mqtt.client as paho
import logging, json, os

log = logging.getLogger("sensor-backend")

# One global instance for the whole app
mqtt_client = paho.Client(paho.CallbackAPIVersion.VERSION2)

# Read config
MQTT_HOST = os.getenv("MQTT_HOST", "mosquitto")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))

# Track connection state
_connection_event = None  # Will be set from main.py

def set_connection_event(event):
    """Called from main.py to signal when MQTT is connected"""
    global _connection_event
    _connection_event = event

def publish_control(device_id: str, payload: dict) -> bool:
    topic = f"devices/{device_id}/control"
    
    # Wait up to 2 seconds for connection if not yet connected
    if not mqtt_client.is_connected():
        if _connection_event:
            log.warning(f"[MQTT] Not connected yet, waiting for connection...")
            if not _connection_event.wait(timeout=2.0):
                log.error(f"[MQTT] Cannot publish to {topic}: Connection timeout")
                return False
        else:
            log.error(f"[MQTT] Cannot publish to {topic}: Client not connected")
            return False

    try:
        payload_str = json.dumps(payload)
        info = mqtt_client.publish(topic, payload_str, qos=1)
        info.wait_for_publish(timeout=1.0)
        log.info(f"[MQTT] Published to {topic}: {payload_str}")
        return True
    except Exception as e:
        log.error(f"[MQTT] Publish failed: {e}")
        return False