#include "mqtt_client.h"

// ==== FreeRTOS ====
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

QueueHandle_t dataQueue;
TaskHandle_t mqttTaskHandle;
// ---- End FreeRTOS ----


void mqtt_data_task(void *pvParameters);

// ==== Setup ====
void setup_mqtt() {
  // Size = 200 items (allows for ~2 seconds of buffer at 100Hz if wifi lags)
  dataQueue = xQueueCreate(200, sizeof(TimeStampedAccelData));
  if (dataQueue == NULL) {
    ESP_LOGE("SENSOR","Error creating MQTT Queue");
  }

  // Stack size set to 10240 words (40KB) because ArduinoJson requires significant memory
  xTaskCreatePinnedToCore(
    mqtt_data_task,   // Function
    "MQTT_Task",      // Name
    16240,            // Stack size (important for large JSON docs)
    NULL,             // Parameters
    1,                // Priority (1 is standard, higher numbers = higher priority)
    &mqttTaskHandle,  // Handle
    1                 // Core ID
  );
}

// ==== Data Transmission ====
const int BUFFER_SIZE = 100; 
char json_transmit_buffer[5120]; 

struct BatchedAccelData {
  int cnt = 0; 
  uint64_t timestamp; 
  uint64_t interval; 
  float ax[BUFFER_SIZE], ay[BUFFER_SIZE], az[BUFFER_SIZE];
};

/**
 * It simply pushes the data to the queue and returns.
 */
void transmit_accelerometer_data(const TimeStampedAccelData &accel_data) {
  if (dataQueue != NULL) {
    // PortMAX_DELAY waits indefinitely for space, 
    // change to 0 to drop packets if queue is full.
    xQueueSend(dataQueue, &accel_data, (TickType_t) 0); 
  }
}

/**
 * Handles batching, serialization, and publishing.
 */
void mqtt_data_task(void *pvParameters) {
  TimeStampedAccelData incoming_data;
  BatchedAccelData local_batch;
  local_batch.cnt = 0;

  DynamicJsonDocument out(16384); 

  for (;;) {
    // Wait for data from queue
    if (xQueueReceive(dataQueue, &incoming_data, portMAX_DELAY) == pdPASS) {
      
      // Initialize batch timestamp on first entry
      if(local_batch.cnt == 0) {
        local_batch.timestamp = incoming_data.timestamp;
        local_batch.interval = MS_INTERVAL; 
      }
      ESP_LOGE("SENSOR","Received data with timestamp: %llu", local_batch.timestamp);
      // Add to local batch
      local_batch.ax[local_batch.cnt] = incoming_data.ax;
      local_batch.ay[local_batch.cnt] = incoming_data.ay;
      local_batch.az[local_batch.cnt] = incoming_data.az;
      local_batch.cnt++;

      if(local_batch.cnt >= BUFFER_SIZE) {
        // Clear previous JSON data
        out.clear();

        out["id"] = device_id;
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
        serializeJson(out, json_transmit_buffer);
        
        // Pass the finalized JSON string to the mesh networking logic
        send_mesh_packet(json_transmit_buffer);

        // Reset Batch
        local_batch.cnt = 0;

        // Yield test to fix CPU Core 1 Traffic ---- For Testing, if Siang doesn't want to do this for PROD we'll revert and try and solve differently.
        //vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
  }
}
