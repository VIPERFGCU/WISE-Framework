#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#include "ntp_client.h"
#include "Accelerometer.h"
#include "mqtt_client.h"
#include "ntp_sensor_node.h"
#include "ntp_mesh.h"

extern const int MS_INTERVAL = 10; // 10ms per record

hw_timer_t * timer = NULL;

// Handle for the task running on Core 0
TaskHandle_t dataRecorderTask;

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
  bool timer_fired = false;
  while(true) {
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

void ntp_sensor_node_init(){

  // Guard incase of recalling( Mesh accidently disconnects )
  static bool is_initialized = false;
  if (is_initialized) return; 
  is_initialized = true;
  // End guard

  ntp_setup();
  init_accelerometer();
  setup_mqtt();

  // Create the data recording task on Core 0
  xTaskCreatePinnedToCore(
    mqttTask,       // Function to implement the task
    "MQTT_Task",    // Name of the task
    10000,          // Stack size in words (increased for JSON ops)
    NULL,           // Task input parameter
    1,              // Priority of the task
    &dataRecorderTask,// Task handle
    0);             // Core where the task should run (0)

  
  timer = timerBegin(0, 80, true); 
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, MS_INTERVAL * 1000, true);
  timerAlarmEnable(timer);

  Serial.println("Setup Complete");
}

