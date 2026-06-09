#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==== MQTT Configuration ====
#define MQTT_ENDPOINT(base, deviceID, suffix) (String(base) + "/" + deviceID + "/" + suffix)

const char* MQTT_SERVER = "10.100.100.1";
const int MQTT_PORT = 1883;

// Keep strings as String objects globally to manage memory automatically
String gateway_topic = MQTT_ENDPOINT("mesh", "", ""); 

// Global Topic Strings
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

// FreeRTOS Handles
QueueHandle_t dataQueue;
TaskHandle_t mqttTaskHandle;

// Function Prototypes
void reconnect();
void assign_id();
void on_message_recieved(char* topic, byte* payload, unsigned int length);
void subscribe_to_topics(String DEVICE_ID);
void mqtt_data_task(void *pvParameters); // New Task Prototype

// IDs
bool id_assigned = false;

// ==== Setup ====
void setup_mqtt() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(on_message_recieved);
  client.setBufferSize(5120); 
  
  reconnect();
  assign_id();

  // 1. Create the Queue
  // Size = 200 items (allows for ~2 seconds of buffer at 100Hz if wifi lags)
  // Item Size = Size of one data sample struct
  dataQueue = xQueueCreate(200, sizeof(TimeStampedAccelData));
  if (dataQueue == NULL) {
    Serial.println("Error creating MQTT Queue");
  }

  // 2. Create the Task
  // Stack size set to 10240 words (40KB) because ArduinoJson requires significant memory
  xTaskCreatePinnedToCore(
    mqtt_data_task,   // Function
    "MQTT_Task",      // Name
    16240,            // Stack size (important for large JSON docs)
    NULL,             // Parameters
    1,                // Priority (1 is standard, higher numbers = higher priority)
    &mqttTaskHandle,  // Handle
    1                 // Core ID (Run on Core 1, leave Core 0 for Wifi/Radio)
  );
}

// ==== Topic Management & Message Handling (Unchanged) ====
void subscribe_to_topics(String DEVICE_ID) {
  if (device_endpoint_str.length() > 0) {
      client.unsubscribe(device_endpoint_str.c_str());
      client.unsubscribe(T_CONTROL.c_str());
      client.unsubscribe(T_STATUS.c_str());
      client.unsubscribe(T_DATA.c_str());
      client.unsubscribe(T_HB.c_str());
  }
  device_endpoint_str = MQTT_ENDPOINT("mesh", DEVICE_ID, "command");
  T_CONTROL = "devices/" + DEVICE_ID + "/control";
  T_STATUS  = "devices/" + DEVICE_ID + "/status";
  T_DATA    = "devices/" + DEVICE_ID + "/data";
  T_HB      = "devices/" + DEVICE_ID + "/heartbeat";
   
  client.subscribe(T_CONTROL.c_str());
  client.subscribe(device_endpoint_str.c_str());
  client.subscribe(T_STATUS.c_str());
  client.subscribe(T_DATA.c_str());
  client.subscribe(T_HB.c_str());
}

void on_message_recieved(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<512> doc; 
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) return;

  const char* type = doc["type"];
  if (!type) return; 
  Serial.println("Type recieved");
  if (strcmp(type, "set_id") == 0) {
    sensor_id = doc["payload"].as<String>();
    subscribe_to_topics(sensor_id);
    id_assigned = true;
    Serial.println("Id assigned");
  } 
}

void assign_id() {
  // If we already have a non-MAC ID, skip
  if(sensor_id != "" && sensor_id.length() < 17) { 
    id_assigned = true;
    return; 
  }

  // 1. Request assignment using MAC address as temporary identifier
  String temp_id = WiFi.macAddress();
  
  StaticJsonDocument<200> out;
  out["id"] = temp_id;
  out["type"] = "client_assignment"; 

  size_t n = serializeJson(out, json_transmit_buffer);

  subscribe_to_topics(temp_id);

  client.publish(gateway_topic.c_str(), json_transmit_buffer, n);

  Serial.println("Waiting for server to assign integer ID...");

  // 2. Spin Lock: Stay here until id_assigned is true
  while (!id_assigned) {
    client.loop(); 
    if (!client.connected()) {
      reconnect();
      
      subscribe_to_topics(temp_id);

      // Re-publish request if we dropped connection
      client.publish(gateway_topic.c_str(), json_transmit_buffer, n);
    }
    
    delay(10); // Feed the watchdog timer
  }

  Serial.print("ID successfully assigned: ");
  Serial.println(sensor_id);
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32Client-" + WiFi.macAddress();
    if (!client.connect(clientId.c_str())) {
      delay(2000); 
    }
  }
}

// ==== Data Transmission Logic ====
extern const int MS_INTERVAL;
const int BUFFER_SIZE = 100; 

struct BatchedAccelData {
  int cnt = 0; 
  uint64_t timestamp; 
  uint64_t interval; 
  float ax[BUFFER_SIZE], ay[BUFFER_SIZE], az[BUFFER_SIZE];
}; 

// NOTE: transmit_buffer global removed. 
// It is now local to the task to prevent thread conflicts.

/**
 * PRODUCER:
 * This function is now non-blocking and extremely fast. 
 * It simply pushes the data to the queue and returns.
 */
 #ifndef USE_OLD
void transmit_accelerometer_data(const TimeStampedAccelData &accel_data) {
  if (dataQueue != NULL) {
    // PortMAX_DELAY waits indefinitely for space, 
    // change to 0 to drop packets if queue is full.
    xQueueSend(dataQueue, &accel_data, (TickType_t) 0); 
  }
}
#endif // USE_OLD

/**
 * CONSUMER TASK:
 * Handles batching, serialization, and publishing.
 */
void mqtt_data_task(void *pvParameters) {
  TimeStampedAccelData incoming_data;
  BatchedAccelData local_batch; // Local buffer, thread-safe
  local_batch.cnt = 0;

  // We allocate the JSON doc here to keep it alive during the task loop
  // Note: 16KB is large for stack, ensure xTaskCreate stack size is sufficient
  DynamicJsonDocument out(16384); 

  for (;;) {
    // Wait for data from queue
    if (xQueueReceive(dataQueue, &incoming_data, portMAX_DELAY) == pdPASS) {
      
      // Initialize batch timestamp on first entry
      if(local_batch.cnt == 0) {
        local_batch.timestamp = incoming_data.timestamp;
        local_batch.interval = MS_INTERVAL; 
      }

      // Add to local batch
      local_batch.ax[local_batch.cnt] = incoming_data.ax;
      local_batch.ay[local_batch.cnt] = incoming_data.ay;
      local_batch.az[local_batch.cnt] = incoming_data.az;
      local_batch.cnt++;

      // Check if batch is full
      if(local_batch.cnt >= BUFFER_SIZE) {
        if (!client.connected()) {
          reconnect();
        }
        
        // Clear previous JSON data
        out.clear();

        out["id"] = sensor_id;
        out["type"] = "data";
        out["t_start"] = local_batch.timestamp;
        out["interval"] = local_batch.interval;

        JsonArray array = out.createNestedArray("vals");
        
        for(int i = 0; i < local_batch.cnt; i++) {
          JsonArray record = array.createNestedArray();
          record.add(local_batch.ax[i]);
          record.add(local_batch.ay[i]);
          record.add(local_batch.az[i]);
        }

        // Serialize
        size_t n = serializeJson(out, json_transmit_buffer);
        
        // Publish
        client.publish(gateway_topic.c_str(), json_transmit_buffer, n); 
        
        // Important: Keep the client alive
        client.loop();

        // Reset Batch
        local_batch.cnt = 0;
      }
    }
  }
}
