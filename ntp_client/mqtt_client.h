#include <PubSubClient.h> // Author: Nick O'Leary
#include <ArduinoJson.h>

// ==== MQTT ====
#define MQTT_ENDPOINT(base, deviceID, suffix) (String(base) + "/" + deviceID + "/" + suffix)
// --------------
const char* MQTT_SERVER = "10.100.100.1";
const int MQTT_PORT = 1883;

// Fixed: Store String to ensure pointer remains valid
String gateway_topic = MQTT_ENDPOINT("mesh", "", "");
const char* MQTT_GATEWAY = gateway_topic.c_str();

// Increased buffer size for batched data (approx 100 * 40 bytes)
char json_transmit_buffer[5120]; 
const char* device_endpoint;
// ---------------------

WiFiClient espClient;
PubSubClient client(espClient);

String sensor_id;

void reconnect();
void assign_id();
void on_message_recieved(char* topic, byte* payload, unsigned int length);

void setup_mqtt() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(on_message_recieved);
  // Important: Increase MQTT internal buffer to handle large batch packets
  client.setBufferSize(5120); 
  reconnect();
  assign_id();
}

void assign_id() {
  if( sensor_id != "" ) {
    return; // ID is already assigned.
  }
  // Temporaily set the id to be the MAC address
  sensor_id = WiFi.macAddress();

  // Listen for the new id
  device_endpoint = MQTT_ENDPOINT("mesh", sensor_id, "command").c_str(); 
  client.subscribe(device_endpoint);

  // Publish the request to the PI
  StaticJsonDocument<200> out;
  out["id"] = sensor_id;
  out["type"] = "client_assignment";

  size_t n = serializeJson(out, json_transmit_buffer);

  client.publish(MQTT_GATEWAY, json_transmit_buffer, n);
}

/* MQTT reconnect */
void reconnect() {
  while (!client.connected()) {
    if (!client.connect("ESP32Client")) {
      delay(2000); // wait before retry
    }
  }
}

void on_message_recieved(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.f_str());
    return;
  }

  // Access values
  const char* type = doc["type"];

  // Set the new id and update the device endpoint
  if(strcmp(type, "set_id") == 0) { // Fixed strcmp check
    client.unsubscribe(device_endpoint);
    sensor_id = doc["payload"].as<String>();;
    device_endpoint = MQTT_ENDPOINT("mesh", sensor_id, "command").c_str();
    client.subscribe(device_endpoint);
  } else if(strcmp(type, "heartbeat")) {
    //do something here
  }
}

extern const int MS_INTERVAL;
const int BUFFER_SIZE = 100; // Fixed: Changed float to int
struct BatchedAccelData {
  int cnt = 0;  // How full the accel data arrays are
  uint64_t timestamp; // The starting timestamp
  uint64_t interval;  // The time seperation between each timestamp
  float ax[BUFFER_SIZE], ay[BUFFER_SIZE], az[BUFFER_SIZE];
}; // Fixed: Missing semicolon

BatchedAccelData transmit_buffer;

void transmit_accelerometer_data(const TimeStampedAccelData &accel_data) {
  // Initialize batch timestamp on first entry
  if(transmit_buffer.cnt == 0) {
    transmit_buffer.timestamp = accel_data.timestamp;
    transmit_buffer.interval = MS_INTERVAL; // Hardcoded MS_INTERVAL matching main
  }

  // Store data in buffer
  transmit_buffer.ax[transmit_buffer.cnt] = accel_data.ax;
  transmit_buffer.ay[transmit_buffer.cnt] = accel_data.ay;
  transmit_buffer.az[transmit_buffer.cnt] = accel_data.az;
  transmit_buffer.cnt++;

  // Only send if buffer is full
  if(transmit_buffer.cnt < BUFFER_SIZE) {
    return;
  }
  
  // Calculate size needed: 100 records * ~40 bytes/record + overhead
  DynamicJsonDocument out(8192); // Using Dynamic for heap allocation on large batches
  out["id"] = sensor_id;
  out["type"] = "data";
  out["t_start"] = transmit_buffer.timestamp;
  out["interval"] = transmit_buffer.interval;

  // Send out the x,y,z data
  JsonArray array = out.createNestedArray("vals");
  
  // Iterate through the batch to populate JSON
  for(int i = 0; i < transmit_buffer.cnt; i++) {
    JsonArray record = array.createNestedArray();
    record.add(transmit_buffer.ax[i]);
    record.add(transmit_buffer.ay[i]);
    record.add(transmit_buffer.az[i]);
    // Note: We don't need individual timestamps if we have t_start + interval
  }

  // Transmit the data
  size_t n = serializeJson(out, json_transmit_buffer);
  
  client.publish(MQTT_GATEWAY, json_transmit_buffer, n); 
  
  // Reset buffer count
  transmit_buffer.cnt = 0;
}
