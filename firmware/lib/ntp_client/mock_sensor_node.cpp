#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#include "ntp_client.h"
// #include "Accelerometer.h" 
#include "mqtt_client.h"
#include "mock_sensor_node.h"
#include "ntp_mesh.h"
#include "mock_ntp_client.h"

extern const int MS_INTERVAL = 10; // 10ms per record

hw_timer_t * mock_timer = NULL;
TaskHandle_t mockDataRecorderTask;

float mock_raw_data[3];
bool mock_isr_timer_fired = false;
portMUX_TYPE mock_timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onMockTimer() {
  portENTER_CRITICAL_ISR(&mock_timerMux);
  mock_isr_timer_fired = true;
  portEXIT_CRITICAL_ISR(&mock_timerMux);
  
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

// Dedicated Task for MQTT (Core 0)
void mockMqttTask(void * parameter) {
  bool timer_fired = false;
  while(true) {
    // Process queue
    portENTER_CRITICAL(&mock_timerMux);
    timer_fired = mock_isr_timer_fired;
    mock_isr_timer_fired = false;
    portEXIT_CRITICAL(&mock_timerMux);
    
    if (timer_fired) {
      // INJECT MOCK DATA
      mock_raw_data[0] = (float)(esp_random() % 100) / 10.0;
      mock_raw_data[1] = (float)(esp_random() % 100) / 10.0;
      mock_raw_data[2] = (float)(esp_random() % 100) / 10.0;

      TimeStampedAccelData A;
      create_timestamped_accel_data(A, mock_raw_data[0], mock_raw_data[1], mock_raw_data[2]);

      transmit_accelerometer_data(A);
    }
    
    vTaskDelay(1); 
  }
}

void mock_sensor_node_init(){
  static bool is_initialized = false;
  if (is_initialized) return; 
  is_initialized = true;

  Serial.println("MOCK MODE ENABLED: Bypassing physical hardware.");

// ntp_setup();          // <-- PRODUCTION LEFT OUT 
  mock_ntp_setup();        // <-- MOCK
  setup_mqtt();

  xTaskCreatePinnedToCore(
    mockMqttTask,       
    "Mock_MQTT_Task",    
    10000,          
    NULL,           
    1,              
    &mockDataRecorderTask,
    0);             

  mock_timer = timerBegin(0, 80, true); 
  timerAttachInterrupt(mock_timer, &onMockTimer, true);
  timerAlarmWrite(mock_timer, MS_INTERVAL * 1000, true);
  timerAlarmEnable(mock_timer);

  Serial.println("Mock Setup Complete");
}