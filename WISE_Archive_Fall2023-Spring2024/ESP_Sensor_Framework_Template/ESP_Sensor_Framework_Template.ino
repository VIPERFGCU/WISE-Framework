/**
 * @file ESP_Sensor_Framework_Template.ino
 * @brief Framework for running a high-rate hardware timer-based sensor polling scheme on core 0 and a low-rate sensor polling scheme on core 1, with data transmission to InfluxDB or SD Card handled by core 1 and time accuracy updates using GPS handled by core 0.
 * @version 4
 * @authors Jordan Kooyman and Andrew Bryan
 *
 * This framework is designed to collect sensor data at different rates and transmit the data to either an InfluxDB database or an SD card. It requires a WiFi or GPS connection to start collecting data.
 *
 * The framework consists of the following main components:
 *
 * - High-rate interrupt-driven sensor polling scheme running on core 0, implemented in SensorsFast.cpp
 * - Low-rate sensor polling scheme running on core 1, implemented in SensorsSlow.cpp
 * - Data transmission to InfluxDB and/or SD card handled by core 1
 * - Time accuracy updates using GPS
 *
 * @note Check the Configuration.h file for parameters that must be updated to get this basic example framework operational.
 * @note Implement sensors that run multiple times per second in SensorsFast.cpp.
 * @note Implement sensors that run once every X seconds in SensorsSlow.cpp.
 * @note Add all sensor setup parameters in SensorConfig.h.
 */

// Core Libraries
#include "esp_timer.h"  // For sensor polling timers
#include <WiFiMulti.h>  // For multi-core operations and WiFi
#include "time.h"       // For epoch-time tracking
#include <SPI.h>
#include <Wire.h>  // I2C/StemmaQT Devices
#include <Adafruit_NeoPixel.h>

// OLED Display
// #include <Adafruit_GFX.h>
// #include <Adafruit_SH110X.h>

// InfluxDB
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// MQTT Client
#include <ESP32HTTPUpdateServer.h>
#include <EspMQTTClient.h>  // By Patrick Lapointe

// SD Card
#include <FS.h>
#include <SD.h>

// Sensor Libraries
#include <TinyGPSPlus.h>          // GPS NMEA Interpreter for GPS Module on Serial Connection
#include <Adafruit_ISM330DHCX.h>  // Accelerometer/Gyro Data (from Adafruit LSM6DS library)

// Local Libraries
#include "Code/Prototypes.h"
#include "Code/Configuration.h"
#include "Code/SensorConfig.h"
#include "Code/Functions.cpp"
#include "Code/SensorsFast.cpp"
#include "Code/SensorsSlow.cpp"


/**
 * @brief Arduino setup function.
 *
 * This function is the entry point of the Arduino program and is called once
 * during the initial startup sequence. It performs the following tasks:
 *
 * 1. Initializes the NeoPixel LED (if `HasNeopixel` is defined).
 * 2. Initializes the OLED display (if `OLEDDebugging` is defined).
 * 3. Initializes the serial communication (if `SerialDebugMode` is defined).
 * 4. Initializes the GPS serial communication.
 * 5. Sets the initial system time using GPS or the internet (via `setTime()`).
 * 6. Connects to the WiFi network for operation (via `setWifiMultiConfig()`).
 * 7. Configures the InfluxDB client (if `InfluxLogging` is defined).
 * 8. Configures the SD card logging (if `SDLogging` is defined).
 * 9. Configures the attached sensors (via `setIsm330Config()`).
 * 10. Starts the low-rate and high-rate sensor timers.
 * 11. Creates a mutex for the InfluxDB client (if `InfluxLogging` is defined).
 * 12. Sets up the GPS module PPS (Pulse Per Second) time synchronization.
 *
 * The function also performs various initialization checks and displays status
 * messages on the serial monitor, OLED display, and NeoPixel LED (if available).
 */
void setup() {
#ifdef HasNeopixel
  pixel.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
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
  display.setCursor(0, 0);
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
  if (!SD.begin()) {  // From SD Card Example
#ifdef SerialDebugMode
    Serial.println("Card Mount Failed");
#endif
#ifdef HasNeopixel
    pixel.setPixelColor(0, pixel.Color(255, 75, 75));
    pixel.show();
#endif
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
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
  pinMode(GPS_PPS_PIN, INPUT_PULLDOWN);  // Need a pull-down mode (not available in Arduino but is in ESP-IDF)
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), GPS_PPS_ISR, RISING);

#ifdef HasNeopixel
  pixel.setPixelColor(0, pixel.Color(0, 100, 0));
  pixel.show();
#endif
}


/**
 * @brief Arduino loop function.
 *
 * This function is the main loop of the Arduino program and is called repeatedly
 * after the `setup()` function. It performs the following tasks:
 *
 * 1. Prints debug information to the serial monitor (if `SerialDebugMode` is defined),
 *    including WiFi signal strength, current time, and InfluxDB connection status.
 * 2. Prints the current time to the OLED display every 10 seconds (if `OLEDDebugging`
 *    is defined).
 * 3. Performs GPS time resynchronization if the `GPSSync` flag is set.
 * 4. Updates the stored system time for GPS interrupt usage.
 * 5. Transmits the Influx buffer if the total data points exceed a certain percentage
 *    of the batch size (if `InfluxLogging` is defined). Repeats until buffers emptied,
 *    or the sequential transmit limit is reached.
 * 6. Handles client write errors (if `InfluxLogging` is defined).
 * 7. Checks if any low-rate sensors should be polled again.
 * 8. Keeps the MQTT connection alive by calling `mqttClient.loop()`.
 *
 * The loop function continuously runs and handles various tasks related to data
 * acquisition, transmission, and time synchronization.
 */
void loop() {
#ifdef SerialDebugMode
  Serial.print("Wifi Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.print(" - Time (seconds): ");
  Serial.print(getSeconds());
  Serial.print("  -  Loop Test - Core ");
  Serial.print(xPortGetCoreID());
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
  if (getSeconds() % 10)
    display.println(getSeconds());
  display.display();
#endif


  // GPS Time Resync
  if (GPSSync) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    GPSSync = false;
    tv.tv_sec += PPSOffsetSeconds;  // Adjust seconds as needed

    // Calculate how long it took since the GPS PPS interrupt to run this, then reset the clock to zero + that offset;
    unsigned long us = micros();
    tv.tv_usec = us - GPS_us + PPSOffsetMicroseconds;

    settimeofday(&tv, nullptr);
  }


#ifdef InfluxLogging
  // Start Transmitting Data if total amount is nearing target Batch Size, repeat until buffer not considered "nearing full"
  char count = 0; // In theory, could get stuck transmitting infinitely if highrate runs too fast; limit how many sequential runs can occur
  while ((slowPointCount + fastPointCount) > (BATCH_SIZE * (StartTransmissionPercentage / 100.0)) && (count < InfluxSequentialTransmitLimit)) {
    transmitInfluxBuffer();
    count++;
  } 

  // Client Write Error Handling
  if (writeError) {
    writeError = false;

#ifdef SerialDebugMode
    Serial.print("Write Error Occured: ");
    Serial.println(client.getLastErrorMessage());
    Serial.print("Full buffer: ");
    Serial.println(client.isBufferFull() ? "Yes" : "No");
#endif

#ifdef OLEDDebugging
    display.print("Write Error Occured: ");
    display.println(client.getLastErrorMessage());
    display.print("Full buffer: ");
    display.println(client.isBufferFull() ? "Yes" : "No");
    display.display();
#endif
  } // end writeError handling
#endif

  // Check if any low-rate sensors should be polled again (no interrupts called)
  checkLowRateSensors();

  // Call the loop function to keep the connection alive
  mqttClient.loop();
}
