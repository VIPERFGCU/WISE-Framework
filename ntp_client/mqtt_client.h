#include <WiFi.h> // Required for WiFi.macAddress()
#include <PubSubClient.h> 
#include <ArduinoJson.h>

// ==== MQTT Configuration ====
#define MQTT_ENDPOINT(base, deviceID, suffix) (String(base) + "/" + deviceID + "/" + suffix)

const char* MQTT_SERVER = "10.100.100.1";
const int MQTT_PORT = 1883;

// Keep strings as String objects globally to manage memory automatically
String gateway_topic = MQTT_ENDPOINT("mesh", "", ""); 

// Global Topic Strings (Initialized empty, populated in subscribe_to_topics)
String T_CONTROL;
String T_STATUS;
String T_DATA;
String T_HB;
String device_endpoint_str; 

// Buffers
char json_transmit_buffer[5120]; 

// Network Objects
WiFiClient espClient;
PubSubClient client(espClient);

String sensor_id;

// Function Prototypes
void reconnect();
void assign_id();
void on_message_recieved(char* topic, byte* payload, unsigned int length);
void subscribe_to_topics(String DEVICE_ID);

// ==== Setup ====
void setup_mqtt() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(on_message_recieved);
  
  // Important: Increase MQTT internal buffer to handle large batch packets
  client.setBufferSize(5120); 
  
  reconnect();
  
  // FIXED: Do not subscribe here. 
  // We don't know the ID or topics yet. 
  // assign_id() will handle the logic to define topics and subscribe.
  assign_id();
}

// ==== Topic Management ====
void subscribe_to_topics(String DEVICE_ID) {
  // 1. Unsubscribe from OLD topics if they exist
  if (device_endpoint_str.length() > 0) {
      client.unsubscribe(device_endpoint_str.c_str());
      client.unsubscribe(T_CONTROL.c_str());
      client.unsubscribe(T_STATUS.c_str());
      client.unsubscribe(T_DATA.c_str());
      client.unsubscribe(T_HB.c_str());
  }
   
  // 2. Update Globals
  // We store the strings globally so the .c_str() pointers remain valid
  device_endpoint_str = MQTT_ENDPOINT("mesh", DEVICE_ID, "command");
  T_CONTROL = "devices/" + DEVICE_ID + "/control";
  T_STATUS  = "devices/" + DEVICE_ID + "/status";
  T_DATA    = "devices/" + DEVICE_ID + "/data";
  T_HB      = "devices/" + DEVICE_ID + "/heartbeat";
   
  // 3. Subscribe to NEW topics
  // Now that the String objects contain data, .c_str() is safe
  client.subscribe(T_CONTROL.c_str());
  client.subscribe(device_endpoint_str.c_str());
  client.subscribe(T_STATUS.c_str());
  client.subscribe(T_DATA.c_str());
  client.subscribe(T_HB.c_str());
}

// ==== Message Handling ====
void on_message_recieved(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<512> doc; // Increased size slightly for safety
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) return;

  const char* type = doc["type"];
  if (!type) return; 

  if (strcmp(type, "set_id") == 0) {
    sensor_id = doc["payload"].as<String>();
    subscribe_to_topics(sensor_id);
  } else if (strcmp(type, "heartbeat") == 0) {
    // Heartbeat logic
  }
}

// ==== ID Assignment ====
void assign_id() {
  if(sensor_id != "") {
    // If we already have an ID, just ensure we are subscribed
    subscribe_to_topics(sensor_id);
    return; 
  }

  // Temporarily set the id to be the MAC address
  sensor_id = WiFi.macAddress();

  // Listen for the new id (using MAC as temporary ID)
  subscribe_to_topics(sensor_id);

  // Publish the request to the Gateway
  StaticJsonDocument<200> out;
  out["id"] = sensor_id;
  out["type"] = "client_assignment";

  size_t n = serializeJson(out, json_transmit_buffer);

  // Use gateway_topic.c_str() directly here for safety
  client.publish(gateway_topic.c_str(), json_transmit_buffer, n);
}

// ==== Connection ====
void reconnect() {
  while (!client.connected()) {
    // Generate a unique client ID based on MAC to prevent broker collisions
    String clientId = "ESP32Client-" + WiFi.macAddress();
    if (!client.connect(clientId.c_str())) {
      delay(2000); 
    }
  }
}

// ==== Data Transmission ====
extern const int MS_INTERVAL;
const int BUFFER_SIZE = 100; 

struct BatchedAccelData {
  int cnt = 0; 
  uint64_t timestamp; 
  uint64_t interval; 
  float ax[BUFFER_SIZE], ay[BUFFER_SIZE], az[BUFFER_SIZE];
}; 

BatchedAccelData transmit_buffer;

void transmit_accelerometer_data(const TimeStampedAccelData &accel_data) {
  // Initialize batch timestamp on first entry
  if(transmit_buffer.cnt == 0) {
    transmit_buffer.timestamp = accel_data.timestamp;
    transmit_buffer.interval = MS_INTERVAL; 
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
   
  DynamicJsonDocument out(16384); // Increased: 100 floats * 3 axes is large
  out["id"] = sensor_id;
  out["type"] = "data";
  out["t_start"] = transmit_buffer.timestamp;
  out["interval"] = transmit_buffer.interval;

  JsonArray array = out.createNestedArray("vals");
   
  for(int i = 0; i < transmit_buffer.cnt; i++) {
    JsonArray record = array.createNestedArray();
    record.add(transmit_buffer.ax[i]);
    record.add(transmit_buffer.ay[i]);
    record.add(transmit_buffer.az[i]);
  }

  size_t n = serializeJson(out, json_transmit_buffer);
   
  client.publish(gateway_topic.c_str(), json_transmit_buffer, n); 
   
  transmit_buffer.cnt = 0;
}
