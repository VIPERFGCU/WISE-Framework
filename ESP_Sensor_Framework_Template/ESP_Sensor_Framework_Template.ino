// ESP_Sensor_Framework_Template.ino  --  V3
// Framework for running a high-rate hardware timer based sensor polling scheme on core 0 and a low rate
// sensor polling scheme on core 1, with data transmission to InfluxDB or SD Card handled by core 1 and
// time accuracy updates using GPS handled by core 0
// Requires WiFi or GPS connection in order to start collecting data
//
// Check the Configuration.h file for parameters that must be updated to get this basic example framework operational
// Implement Sensors that run multiple times per second in SensorsFast.cpp
// Implement Sensors that run once every X seconds in SensorsSlow.cpp
// Add all Sensor setup parameters in SensorConfig.h
//
// Written by: Jordan Kooyman

// Core Libraries
#include "esp_timer.h" // For sensor polling timers
#include <WiFiMulti.h> // For multi-core operations and WiFi
#include "time.h"      // For epoch-time tracking
#include <SPI.h>
#include <Wire.h>      // I2C/StemmaQT Devices
#include <Adafruit_NeoPixel.h>

// OLED Display
// #include <Adafruit_GFX.h>
// #include <Adafruit_SH110X.h>

// InfluxDB
#include <InfluxDbClient.h>
// #include <InfluxDbCloud.h>

// SD Card
#include <FS.h>
#include <SD.h>

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
  #ifdef HasNeopixel
  pixel.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixel.setPixelColor(0, pixel.Color(255, 0, 0));
  pixel.show();
  #endif

  // OLED Display Connection
  #ifdef OLEDDebugging
  display.begin(0x3c, true);
  display.display();
  #endif

  // Debugging Serial Printouts
  #ifdef SerialDebugMode
    Serial.begin(SerialBaudRate);
  #endif

  // GPS Serial Connection
  GPSSerial.begin(GPSBaudRate);

  // Prepare OLED Display for text output
  #ifdef OLEDDebugging
  display.clearDisplay();
  display.display();
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  #endif

  // Set Initial System Epoch Time from GPS or Internet
  setTime();

  // Connect to WiFi for Operation (Wait until Connected)
  setWifiMultiConfig();

  // Configure InfluxDB Client (connection in configuration.h global variables)
  #ifdef InfluxLogging
  setInfluxConfig();
  #endif

  // Configure SD Card Logging
  #ifdef SDLogging
  if(!SD.begin()){ // From SD Card Example
    #ifdef SerialDebugMode
    Serial.println("Card Mount Failed");
    #endif
    #ifdef HasNeopixel
    pixel.setPixelColor(0, pixel.Color(255, 75, 75));
    pixel.show();
    #endif
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    #ifdef SerialDebugMode
    Serial.println("No SD card attached");
    #endif
    #ifdef HasNeopixel
    pixel.setPixelColor(0, pixel.Color(255, 75, 75));
    pixel.show();
    #endif
  }
  #endif

  // Setup Attached Sensors
  setIsm330Config();

  // Start Timers
  startLowRateSensors();
  startHighRateSensors();
  #ifdef InfluxLogging
  InfluxClientMutex = xSemaphoreCreateMutex();
  #endif

  // Setup GPS Module PPS Time Sync
  pinMode(GPS_PPS_PIN, INPUT_PULLDOWN); // Need a pull-down mode (not available in Arduino but is in ESP-IDF)
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), GPS_PPS_ISR, RISING);

  #ifdef HasNeopixel
  pixel.setPixelColor(0, pixel.Color(0, 100, 0));
  pixel.show();
  #endif
}


// Empty and unused loop function (still required),
//  will never be run since core tasks will never complete
void loop()
{
  #ifdef SerialDebugMode
    Serial.print("Wifi Strength: "); Serial.print(WiFi.RSSI());
    Serial.print(" - Time (seconds): "); Serial.print(getSeconds());
    Serial.print("  -  Loop Test - Core "); Serial.print(xPortGetCoreID());
    #ifdef InfluxLogging
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }
    #endif
    delay(200);
  #endif

  // Print out the time once every 10 seconds to prove ESP is still running
  #ifdef OLEDDebugging
  if(getSeconds() % 10)
    display.println(getSeconds());
    display.display();
  #endif


  // GPS Time Resync
  if (GPSSync)
  {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    GPSSync = false;
    tv.tv_sec += PPSOffsetSeconds; // Adjust seconds as needed

    // Calculate how long it took since the GPS PPS interrupt to run this, then reset the clock to zero + that offset;
    unsigned long us = micros();
    tv.tv_usec = us - GPS_us + PPSOffsetMicroseconds; 
    
    settimeofday(&tv, nullptr);
  }
  

  // Update Stored System Time for GPS Interrupt Usage
  // gettimeofday(&tv, nullptr);

  #ifdef InfluxLogging
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
      Serial.print("Write Error Occured: "); Serial.println(client.getLastErrorMessage());
      Serial.print("Full buffer: "); Serial.println(client.isBufferFull() ? "Yes" : "No");
    #endif

    #ifdef OLEDDebugging
      display.print("Write Error Occured: "); display.println(client.getLastErrorMessage());
      display.print("Full buffer: "); display.println(client.isBufferFull() ? "Yes" : "No");
      display.display();
    #endif
  }
  #endif

  // Check if any low-rate sensors should be polled again (no interrupts called)
  checkLowRateSensors();
}

