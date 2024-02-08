// ESP_Sensor_Framework_Template.ino  --  V2
// Framework for running a high-rate hardware timer based sensor polling scheme on core 0 and a low rate
// sensor polling scheme on core 1, with data transmission handled by core 1 and time accuracy updates 
// using GPS handled by core 0
// Requires WiFi connection in order to start collecting data
//
// Check the Configuration.h file for parameters that must be updated to get this basic example framework operational
//
// Written by: Jordan Kooyman

// Core Libraries
#include "esp_timer.h" // For sensor polling timers
#include <WiFiMulti.h> // For multi-core operations and WiFi
#include "time.h"      // For epoch-time tracking
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Sensor Libraries
#include <TinyGPSPlus.h>         // GPS NMEA Interpreter for GPS Module on Serial Connection
#include <Adafruit_ISM330DHCX.h> // Accelerometer/Gyro Data (from Adafruit LSM6DS library)

// Local Libraries
#include "Code/Prototypes.h"
#include "Code/Configuration.h"
#include "Code/SensorConfig.h"
#include "Code/Functions.cpp"
#include "Code/SensorsFast.cpp"
#include "Code/SensorsSlow.cpp"



void setup()
{  
  // Debugging Serial Printouts
  #ifdef SerialDebugMode
    Serial.begin(SerialBaudRate);
  #endif

  // GPS Serial Connection
  GPSSerial.begin(GPSBaudRate);

  // Connect to WiFi for Initial Setup (Wait until Connected)
  setWifiConfig();

  // Set Initial System Epoch Time from Internet
  setenv("TZ", TimeZoneOffset,1);
  setTime();

  // Connect to WiFi for Operation (Wait until Connected)
  setWifiMultiConfig();

  // Configure InfluxDB Client (connection in configuration.h global variables)
  setInfluxConfig();

  // Setup Attached Sensors
  setIsm330Config();

  // Start Timers
  startLowRateSensors();
  startHighRateSensors();
  InfluxClientMutex = xSemaphoreCreateMutex();

  // Setup GPS Module PPS Time Sync
  pinMode(GPS_PPS_PIN, INPUT); // Need a pull-down mode (not available in Arduino but is in ESP-IDF)
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), GPS_PPS_ISR, RISING);
}


// Empty and unused loop function (still required),
//  will never be run since core tasks will never complete
void loop()
{
  #ifdef SerialDebugMode
    Serial.print("Wifi Strength: "); Serial.print(WiFi.RSSI());
    Serial.print(" - Time (seconds): "); Serial.print(getSeconds());
    Serial.print("  -  Loop Test - Core "); Serial.print(xPortGetCoreID());
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }
    delay(200);
    Serial.println(digitalRead(GPS_PPS_PIN));
  #endif

  

  // Update Stored System Time for GPS Interrupt Usage
  gettimeofday(&tv, nullptr);

  // Start Transmitting Data if total amount is nearing target Batch Size
  if ((slowPointCount + fastPointCount) > (BATCH_SIZE * (StartTransmissionPercentage / 100.0)))
  {
    transmitInfluxBuffer();
  }

  // Client Write Error Handling
  if (writeError)
  {
    writeError = false;

    #ifdef SerialDebugMode
      Serial.print("Write Error Occured: ");
      Serial.println(client.getLastErrorMessage());
      Serial.print("Full buffer: ");
      Serial.println(client.isBufferFull() ? "Yes" : "No");
    #endif
  }

  // Check if any low-rate sensors should be polled again (no interrupts called)
  checkLowRateSensors();
}

