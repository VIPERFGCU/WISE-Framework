#include <WiFi.h>
#include <WiFiUdp.h>
#include "ntp_client.h"
#include "Accerometer.h"
#include "mqtt_client.h"

//# define USE_OLD  // the http influxdb handler
//#include "influx_db_handler.h"

// ================== WIFI CONFIGURATION ===================
const char* ssid = "pop-os";
const char* password = "adminnnn";
// ======================================================================
const int MS_INTERVAL = 20; // 20ms per record

QueueHandle_t dataQueue;
const int queueSize = 200; // Increased to accommodate buffering during network lag

hw_timer_t * timer = NULL;

// Handle for the task running on Core 0
TaskHandle_t mqttTaskHandle;

float raw_data[3];
void IRAM_ATTR onTimer() {
  get_accelerometer_data(raw_data);
  TimeStampedAccelData A;
  create_timestamped_accel_data(A, raw_data[0], raw_data[1], raw_data[2]);

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(dataQueue, &A, &xHigherPriorityTaskWoken);

  // If a task was waiting for this data, ensure it switches context if needed
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

// Dedicated Task for MQTT (Core 0)
void mqttTask(void * parameter) {
  setup_mqtt();  

  TimeStampedAccelData receivedData;

  for(;;) {
    // Maintain MQTT connection
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    // Process queue
    if (xQueueReceive(dataQueue, &receivedData, 0) == pdPASS) {
      transmit_accelerometer_data(receivedData);
    }
    
    // Small yield to prevent watchdog trigger if queue is empty for long periods
    vTaskDelay(1); 
  }
}

void setup(){
  Serial.begin(115200);

  // Create queue large enough to hold data while batch is sending
  dataQueue = xQueueCreate(queueSize, sizeof(TimeStampedAccelData));

  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  ntp_setup();
  init_accelerometer();

  // Create the MQTT task on Core 0
  xTaskCreatePinnedToCore(
    mqttTask,       // Function to implement the task
    "MQTT_Task",    // Name of the task
    10000,          // Stack size in words (increased for JSON ops)
    NULL,           // Task input parameter
    1,              // Priority of the task
    &mqttTaskHandle,// Task handle
    0);             // Core where the task should run (0)

  timer = timerBegin(1000000);  // 1Mhz
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, MS_INTERVAL*1000, true, 0);

  Serial.println("Setup Complete");
}

void loop() {
  // Empty: work is done in Timer ISR (Core 1) and MQTT Task (Core 0)
  vTaskDelete(NULL); // Delete the loop task to save resources
}
