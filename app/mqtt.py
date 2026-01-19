# app/mqtt.py
import paho.mqtt.client as paho
import logging, json

log = logging.getLogger("sensor-backend")

# One global instance for the whole app
mqtt_client = paho.Client(paho.CallbackAPIVersion.VERSION2)

def publish_control(device_id: str, payload: dict) -> bool:
    topic = f"devices/{device_id}/control"
    
    if not mqtt_client.is_connected():
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