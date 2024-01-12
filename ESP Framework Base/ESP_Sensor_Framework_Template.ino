// ESP_Sensor_Framework_Template.ino  --  V1
// Framework for running a high-rate hardware timer based sensor polling scheme on core 1 and a low rate
// sensor polling scheme on core 0, with data transmission handled by core 0 and time accuracy updates 
// using GPS handled by core 1
// Requires WiFi connection in order to start
// Written by: Jordan Kooyman


// Core Libraries
#include "esp_timer.h" // For sensor polling timers
#include <WiFiMulti.h> // For multi-core operations and WiFi
#include "time.h"      // For epoch-time tracking
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Sensor Libraries
//#include <TinyGPSPlus.h>         // GPS NMEA Interpreter for GPS Module on Serial Connection
#include <Adafruit_ISM330DHCX.h> // Onboard Accelerometer/Gyro Data (from Adafruit LSM6DS library)

// Local Libraries
#include "Code/Prototypes.h"
#include "Code/Configuration.h"
#include "Code/SensorConfig.h"
#include "Code/Functions.cpp"
#include "Code/SensorsFast.cpp"
#include "Code/SensorsSlow.cpp"
#include "Code/Core0.cpp"
#include "Code/Core1.cpp"


void setup()
{  
  // Connect to WiFi (Wait until Connected)
  setWifiConfig();
  
  
  // Force dual-core operation on ESP32 and start each core's individual setup function

  //create a task that will be executed in the core0OS() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    core0OS,     /* Task function. */
                    "Core 0",    /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(250); 

  //create a task that will be executed in the core1OS() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    core1OS,     /* Task function. */
                    "Core 1",    /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(250); 
}


// Perform one-time initialization and infinite running of core 0 code on core 0
void core0OS(void * pvParameters)
{
  core0Setup();
  while(true)
  { core0Loop(); }
}


// Perform one-time initialization and infinite running of core 1 code on core 1
void core1OS(void * pvParameters)
{
  core1Setup();
  while(true)
  { core1Loop(); }
}


// Empty and unused loop function (still required),
//  will never be run since core tasks will never complete
void loop()
{
    // Do Nothing!
}

