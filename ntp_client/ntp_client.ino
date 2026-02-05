#include <WiFi.h>
#include <WiFiUdp.h>
#include "ntp_client.h"
#include "Accerometer.h"
#include "mqtt_client.h"

//# define USE_OLD  // the http influxdb handler
#include "influx_db_handler.h"

// ================== WIFI CONFIGURATION ===================
const char* ssid = "pop-os";
const char* password = "adminnnn";
// ======================================================================
const int MS_INTERVAL = 20; // 20ms per record

hw_timer_t * timer = NULL;

// Handle for the task running on Core 0
TaskHandle_t mqttTaskHandle;

float raw_data[3];
bool isr_timer_fired = false;
portMUX_TYPE accel_timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&accel_timerMux);
  isr_timer_fired = true;
  portEXIT_CRITICAL_ISR(&accel_timerMux);
  
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // If a task was waiting for this data, ensure it switches context if needed
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

// Dedicated Task for MQTT (Core 0)
void mqttTask(void * parameter) {
  setup_mqtt();

  bool timer_fired = false;
  while(true) {
    // Maintain MQTT connection
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    // Process queue
    portENTER_CRITICAL(&accel_timerMux);
    timer_fired = isr_timer_fired;
    isr_timer_fired = false;
    portEXIT_CRITICAL(&accel_timerMux);
    
    if (timer_fired) {
      get_accelerometer_data(raw_data);
      TimeStampedAccelData A;
      create_timestamped_accel_data(A, raw_data[0], raw_data[1], raw_data[2]);

      transmit_accelerometer_data(A);
    }
    
    // Small yield to prevent watchdog trigger if queue is empty for long periods
    vTaskDelay(1); 
  }
}

void setup(){
  Serial.begin(115200);

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
  timerAlarm(timer, MS_INTERVAL, true, 0);
  timerStart(timer);

  Serial.println("Setup Complete");
}

void loop() {
  // Empty: work is done in Timer ISR (Core 1) and MQTT Task (Core 0)
  vTaskDelete(NULL); // Delete the loop task to save resources
}
