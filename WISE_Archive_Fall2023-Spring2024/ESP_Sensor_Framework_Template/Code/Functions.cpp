/**
 * @file Functions.cpp
 * @brief Contains function prototypes and helper functions for the entire program.
 * @authors Jordan Kooyman and Andrew Bryan
 *
 * This file contains the implementations of various helper functions used throughout
 * the program. These functions are responsible for tasks such as setting up WiFi
 * configurations, synchronizing time, logging data to SD cards or Influx databases,
 * handling interrupts, and more.
 *
 * The functions defined in this file are likely called from other parts of the
 * program, such as the main loop or interrupt service routines.
 */

#ifndef FunctionsCode
#define FunctionsCode


#include "Configuration.h"

#ifdef InfluxLogging
/**
 * @brief Configures the Influx client settings.
 * 
 * This function enables batching and sets the timestamp precision to microseconds.
 * 
 * @return void
 */
void setInfluxConfig() {
  // Enable batching and timestamp precision
  client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::US).batchSize(BATCH_SIZE));
}
#endif

/**
 * @brief Sets the WiFi configuration for a specific network.
 *
 * This function attempts to connect to a WiFi network specified by the `Network`
 * parameter. It first sets the WiFi mode to station mode (`WIFI_STA`), then
 * connects to the network using the provided SSID and password.
 *
 * @param Network An integer specifying the network to connect to:
 *                1 for the primary network, 2 for the secondary network, etc.
 *
 * @return `true` if the connection is successful, `false` otherwise.
 *
 * The function prints debug messages to the serial monitor and OLED display (if
 * available) during the connection process. It waits for a specified amount of
 * time (`WIFICONNECTTIME`) for the connection to be established before giving up.
 */
bool setWifiConfig(int Network) {
  WiFi.mode(WIFI_STA);
  if (Network == 1)
    WiFi.begin(I_WIFI_SSID, I_WIFI_PASSWORD);
  else if (Network == 2)
    WiFi.begin(I_WIFI_SSID2, I_WIFI_PASSWORD2);

#ifdef SerialDebugMode
  Serial.print("Connecting to wifi");
  Serial.print(WiFi.SSID());
#endif
#ifdef OLEDDebugging
  display.print("Connecting to wifi");
  display.print(WiFi.SSID());
  display.display();
#endif

  int cycles = 0;
  while (WiFi.status() != WL_CONNECTED && cycles < (2 * WIFICONNECTTIME)) {
#ifdef SerialDebugMode
    Serial.print(".");
#endif
#ifdef OLEDDebugging
    display.print(".");
    display.display();
#endif
    delay(500);
    cycles++;
  }
  if (cycles >= (2 * WIFICONNECTTIME)) {
#ifdef SerialDebugMode
    Serial.print("Not Connected to wifi");
    Serial.println(WiFi.SSID());
#endif
#ifdef OLEDDebugging
    display.print("Not Connected to wifi");
    display.println(WiFi.SSID());
    display.display();
#endif
    return false;
  }
#ifdef SerialDebugMode
  Serial.print("Connected to wifi");
  Serial.println(WiFi.SSID());
#endif
#ifdef OLEDDebugging
  display.print("Connected to wifi");
  display.println(WiFi.SSID());
  display.display();
#endif
  return true;
}

/**
 * @brief Sets the WiFi configuration using the WiFiMulti library.
 *
 * This function attempts to connect to a WiFi network using the WiFiMulti library.
 * It first disconnects from any existing WiFi connection, sets the WiFi mode to
 * station mode (`WIFI_STA`), and then adds the configured access points to the
 * WiFiMulti object.
 *
 * The function then waits for a connection to be established with one of the
 * added access points. It prints debug messages to the serial monitor and OLED
 * display (if available) during the connection process.
 *
 * If a connection is established successfully, the function sets the NeoPixel
 * color to brownish-red (if available) to indicate the successful connection.
 *
 * @return void
 */
void setWifiMultiConfig() {
  WiFi.disconnect(true, true);
  delay(250);
  WiFi.mode(WIFI_STA);

  wifiMulti.addAP(O_WIFI_SSID, O_WIFI_PASSWORD);
  // wifiMulti.addAP(O_WIFI_SSID2, O_WIFI_PASSWORD2);

#ifdef SerialDebugMode
  Serial.print("Connecting to wifi");
  Serial.print(WiFi.SSID());
#endif
#ifdef OLEDDebugging
  display.print("Connecting to wifi");
  display.print(WiFi.SSID());
  display.display();
#endif
  while (wifiMulti.run() != WL_CONNECTED) {
#ifdef SerialDebugMode
    Serial.print(".");
#endif
#ifdef OLEDDebugging
    display.print(".");
    display.display();
#endif
    delay(500);
  }
#ifdef SerialDebugMode
  Serial.print("Connected to wifi");
  Serial.println(WiFi.SSID());
#endif
#ifdef OLEDDebugging
  display.print("Connected to wifi");
  display.println(WiFi.SSID());
  display.display();
#endif
#ifdef HasNeopixel
  pixel.setPixelColor(0, pixel.Color(150, 75, 75));
  pixel.show();
#endif
}

/**
 * @brief Sets the system time using various methods.
 *
 * This function attempts to set the system time using the following methods:
 *
 * 1. If a GPS connection is available, it tries to retrieve the time from the GPS
 *    module and set the system time accordingly.
 * 2. If the GPS time is not available or fails, it attempts to connect to a WiFi
 *    network and perform an NTP time synchronization.
 * 3. If the NTP time synchronization fails on the primary network, it attempts
 *    to connect to a secondary network and perform the NTP time synchronization.
 * 4. If the NTP time synchronization fails on the secondary network, it retries
 *    the GPS and waits until it can get the time set
 *
 * Neopixel Colors:
 * - Purple (255, 0, 255): Before setting the timezone.
 * - Dark Blue (0, 0, 125): Connecting to the primary WiFi network for initial setup.
 * - Teal (0, 125, 125): Disconnecting from the primary network and connecting to the secondary network for time synchronization.
 * - White (255, 255, 255): Indicating the usage of GPS time for setting the system time.
 * - Yellow (255, 255, 0): Completion of the time synchronization process.
 *
 * The function uses the `setWifiConfig` function to connect to the WiFi networks
 * and the `tryTimeSync` function to perform the NTP time synchronization.
 *
 * If the GPS time is used, the function sets the NeoPixel color (if available)
 * to indicate that the GPS time is being used. If the NTP time synchronization
 * is successful, the NeoPixel color is set to indicate a successful time sync.
 *
 * The function also prints debug messages to the serial monitor and OLED display
 * (if available) during the time synchronization process.
 *
 * @return void
 */
void setTime() {
#ifdef HasNeopixel
  pixel.setPixelColor(0, pixel.Color(255, 0, 255));
  pixel.show();
#endif
  setenv("TZ", TimeZoneOffset, 1);

  bool useGPSTime = true;

  if (!checkGPSTime()) {
// Network 1
#ifdef HasNeopixel
    pixel.setPixelColor(0, pixel.Color(0, 0, 125));
    pixel.show();
#endif
    // Connect to WiFi for Initial Setup (Wait until Connected)
    bool connected = setWifiConfig();

    if (!tryTimeSync(connected))  // Attempt NTP Time Sync on primary network
    {
// Network 2
#ifdef HasNeopixel
      pixel.setPixelColor(0, pixel.Color(0, 125, 125));
      pixel.show();
#endif
      WiFi.disconnect(true, true);
      delay(250);
      connected = setWifiConfig(2);  // Use alternate network to get time from
      useGPSTime = !tryTimeSync(connected);
    } else
      useGPSTime = false;  // First time sync attempt worked
  }
  if (useGPSTime) {
#ifdef HasNeopixel
    pixel.setPixelColor(0, pixel.Color(255, 255, 255));
    pixel.show();
#endif
#ifdef SerialDebugMode
    Serial.println("Using GPS Time");
#endif
#ifdef OLEDDebugging
    display.println("Using GPS Time");
    display.display();
#endif
    setUnixtime(getGPSTime());
  }

// Time Sync completed
#ifdef HasNeopixel
  pixel.setPixelColor(0, pixel.Color(255, 255, 0));
  pixel.show();
#endif
}


/**
 * @brief Attempts to synchronize the system time using NTP.
 *
 * This function tries to synchronize the system time using the Network Time
 * Protocol (NTP). It first checks if a WiFi connection is available. If not,
 * it returns `false` immediately.
 *
 * If a WiFi connection is available, the function attempts to synchronize the
 * time using the `timeSync` function from the InfluxDB client library. It
 * performs multiple attempts (up to `I_WIFI_ATTEMPT_COUNT_LIMIT`) until the
 * time is successfully synchronized or the attempt limit is reached.
 *
 * @param WiFi_Connected A boolean indicating whether a WiFi connection is
 *                       available or not.
 *
 * @return `true` if the time synchronization is successful, `false` otherwise.
 *
 * The function prints debug messages to the serial monitor and OLED display
 * (if available) during the time synchronization process.
 */
bool tryTimeSync(bool WiFi_Connected) {
  if (!WiFi_Connected)
    return false;
  int attemptCount = 1;
  struct timeval tv;
  // Update Stored System Time
  gettimeofday(&tv, nullptr);
#ifdef SerialDebugMode
  Serial.print("Time: ");
  Serial.println(getSeconds());
#endif
#ifdef OLEDDebugging
  display.print("Time: ");
  display.println(getSeconds());
  display.display();
#endif

  while (getSeconds() < 1000) {  // Retry Set Initial System Epoch Time from Internet
#ifdef SerialDebugMode
    Serial.print("Attempt ");
    Serial.println(attemptCount);
#endif
#ifdef OLEDDebugging
    display.print("Attempt ");
    display.println(attemptCount);
    display.display();
#endif
    timeSync(TimeZoneOffset, ntpServer, ntpServer2);  // Influx Client Function
    // Update Stored System Time
    gettimeofday(&tv, nullptr);
    attemptCount++;
    if (attemptCount > I_WIFI_ATTEMPT_COUNT_LIMIT)
      return false;
  }
  return true;
}

/**
 * @brief Checks if the GPS module has a valid time.
 *
 * This function waits for a short period (1 second) for GPS data to become
 * available. If GPS data is received, it parses the data using the TinyGPS++
 * library and checks if the time information is valid.
 *
 * @return `true` if the GPS module has a valid time, `false` otherwise.
 *
 * The function first waits for up to 1 second for GPS data to become available
 * in the serial buffer. If data is available, it reads and parses the data using
 * the `gps.encode()` function from the TinyGPS++ library.
 *
 * After parsing the data, the function checks if the `gps.time` object has a
 * valid time by calling the `isValid()` method. If the time is valid, it returns
 * `true`, indicating that the GPS module has a valid time.
 *
 * If no GPS data is received within the 1-second timeout, or if the received
 * data does not contain a valid time, the function returns `false`.
 */
bool checkGPSTime() {
  // Wait until GPS data is available or timeout (1 second)
  unsigned long start = millis();
  while (GPSSerial.available() < 10 && millis() - start < 1000) {
    delay(100);
  }

  // If there is new data, parse it
  if (GPSSerial.available() > 0) {
    while (GPSSerial.available()) {
      gps.encode(GPSSerial.read());
    }
    // Check if GPS has valid time
    if (gps.time.isValid()) {
      return true;  // GPS has current time
    }
  }

  // No valid time found
  return false;
}

/**
 * @brief Retrieves the current time from the GPS module.
 *
 * This function reads data from the GPS serial buffer, parses the GPS data
 * using the TinyGPS++ library, and extracts the current date and time. It then
 * converts the date and time components into a Unix timestamp (epoch time) and
 * returns it.
 *
 * @return The current time as a Unix timestamp (epoch time) in seconds, or 0 if
 *         no valid GPS data is available.
 */
int32_t getGPSTime() {
  // Ensure GPS data is available
  if (!GPSSerial.available()) {
    return 0;  // Return 0 if no data available
  }

  // Create a variable to hold GPS time
  int32_t gpsTime = 0;

  // Parse GPS data
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
    if (gps.date.isValid() && gps.time.isValid()) {
      // Create a tm struct to hold time components
      struct tm t;
      t.tm_year = gps.date.year() - 1900;
      t.tm_mon = gps.date.month() - 1;
      t.tm_mday = gps.date.day();
      t.tm_hour = gps.time.hour();
      t.tm_min = gps.time.minute();
      t.tm_sec = gps.time.second();

      // Convert tm struct to epoch time
      gpsTime = mktime(&t);
      break;  // Exit loop once time is obtained
    }
  }

  return gpsTime;
}

/**
 * @brief Sets the system time using a Unix timestamp.
 *
 * This function sets the system time by converting the provided Unix timestamp
 * (epoch time) to a `timeval` struct and calling `settimeofday`.
 *
 * @param unixtime The Unix timestamp (epoch time) in seconds.
 *
 * @return The return value of `settimeofday`. On success, it returns 0;
 *         otherwise, it returns -1 and sets `errno` to indicate the error.
 *
 * @note This function is based on the solution provided in the following GitHub
 *       issue: https://github.com/espressif/arduino-esp32/issues/1444
 */
int setUnixtime(int32_t unixtime) {  // From https://github.com/espressif/arduino-esp32/issues/1444
  timeval epoch = { unixtime, 0 };
  return settimeofday((const timeval*)&epoch, 0);
}


/**
 * @brief Interrupt Service Routine (ISR) for the GPS PPS (Pulse Per Second) signal.
 *
 * This function is designed to be as simple and fast as possible to maintain the
 * high accuracy of the PPS pulse. It is called whenever the GPS PPS interrupt is
 * triggered.
 *
 * The function performs the following tasks:
 *
 * 1. Records the current time in microseconds using `micros()` and stores it in
 *    the `GPS_us` variable.
 * 2. Sets the `GPSSync` flag to `true`, indicating that a GPS synchronization
 *    event has occurred.
 *
 * If `SerialDebugMode` and `InterruptDebugging` are defined, it prints a debug
 * message to the serial monitor.
 *
 * @note This function is marked with `ARDUINO_ISR_ATTR` to ensure it is loaded for optimal
 *       ISR use.
 *
 * @note The implementation is inspired by the approach used in the following
 *       Arduino forum thread: https://forum.arduino.cc/t/super-accurate-1ms-yr-gps-corrected-rtc-clock-without-internet-ntp/640518
 *
 * @return void
 */
void ARDUINO_ISR_ATTR GPS_PPS_ISR() {
  GPS_us = micros();
  GPSSync = true;

#ifdef SerialDebugMode
#ifdef InterruptDebugging
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.print("GPS PPS Reset");
  Serial.println();
  Serial.println();
#endif
#endif
}


/**
 * @brief Get the current epoch/Unix time.
 *
 * This function retrieves the current epoch/Unix time, which is the number of
 * seconds elapsed since January 1, 1970, 00:00:00 UTC. It is used for timestamping
 * data points in InfluxDB.
 *
 * @return The current epoch/Unix time as an unsigned long long integer.
 *         If the local time cannot be obtained, it returns 0.
 *
 * @note This function is based on the example from RandomNerdTutorials:
 *       https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/
 */
unsigned long long getTime() {  // unsigned long long type to match the type used by Point setTime function in InfluxDbClient
  time_t now;                   // https://cplusplus.com/reference/ctime/time_t/
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
#ifdef SerialDebugMode
    Serial.println("Failed to obtain time");
#endif
#ifdef OLEDDebugging
    display.println("Failed to obtain time");
    display.display();
#endif
    return (0);
  }
  time(&now);
  return now;
}


/**
 * @brief Get the number of seconds since the epoch.
 *
 * This function retrieves the number of seconds elapsed since January 1, 1970,
 * 00:00:00 UTC.
 *
 * @return The number of seconds since the epoch as an unsigned long long integer.
 */
unsigned long long getSeconds() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}


/**
 * @brief Get the number of microseconds since the epoch.
 *
 * This function retrieves the number of microseconds elapsed since January 1, 1970,
 * 00:00:00 UTC.
 *
 * @return The number of microseconds since the epoch as an unsigned long long integer.
 */
unsigned long long getuSeconds() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_usec;
}



#ifdef InfluxLogging
/**
 * @brief Transmits the Influx buffer to the database.
 *
 * This function transmits the data points stored in the Influx buffer to the
 * database. It performs the following steps:
 *
 * 1. Acquires the `InfluxClientMutex` mutex to ensure exclusive access to the
 *    Influx client.
 * 2. Flushes the Influx client buffer, forcing the buffered data points to be
 *    sent to the database.
 * 3. Releases the `InfluxClientMutex` mutex.
 * 4. Resets the `fastPointCount` and `slowPointCount` counters.
 * 5. Iterates over the `fast_datapoints` array and writes any queued fast-rate
 *    data points to the database using the Influx client.
 * 6. Resets the `fastPointCountAlt` counter.
 * 7. Iterates over the `fast_datapoints` array and deallocates the memory used
 *    by the data points.
 *
 * If `SerialDebugMode` and `TransmitDetailDebugging` are defined, debug messages
 * are printed to the serial monitor.
 *
 * @note If there is an error writing a data point to the database, the
 *       `writeError` flag is set.
 *
 * @return void
 *
 * @return void
 */
void transmitInfluxBuffer() {
#if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
  Serial.print(xPortGetCoreID());
  Serial.print(" Core - Transmit Buffer");
#endif
  // Transmit Buffered Datapoints
  if (xSemaphoreTake(InfluxClientMutex, (TickType_t)1000) == pdTRUE) {

#if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.print("Transmit take Mutex");
#endif

    // Force Influx Client buffer to send
    client.flushBuffer();

#if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.print("Transmit Done");
#endif

#if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
      Serial.print("Transmit Return Mutex");
      Serial.print(" Aux Buffer Size: ");
      Serial.print(fastPointCountAlt);
#endif

      for (int i = 0; i < fastPointCountAlt; i++) {                         // Buffer Queued Fast Rate Datapoints
        writeError = writeError || client.writePoint(*fast_datapoints[i]);  // Dereference pointer to get the Point object
      }

#if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
      Serial.print(" - Queued Buffer into Influx ");
#endif

    fastPointCount = fastPointCountAlt;
    slowPointCount = 0;

    xSemaphoreGive(InfluxClientMutex);  // After accessing the shared resource give the mutex and allow other processes to access it
  }
  

  fastPointCountAlt = 0;
  // Iterate through the entire array and delete each element
  for (int i = 0; i < BATCH_SIZE; i++) {
    if (fast_datapoints[i] != nullptr) {  // Check if the element is not already deleted
      delete fast_datapoints[i];          // Delete the element
      fast_datapoints[i] = nullptr;       // Set the pointer to nullptr
    }
  }


#if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
  Serial.println("Emptied Buffer, Done");
#endif
} // end transmitInfluxBuffer()

/**
 * @brief Logs data to an Influx database.
 *
 * This function logs a data point to an Influx database. It creates a Point object
 * and adds the data fields to it. If this is the final data point, the Point is
 * added to the transmission buffer or an alternate buffer if the transmission
 * buffer is not available.
 *
 * @param uS The microsecond part of the timestamp.
 * @param S The second part of the timestamp.
 * @param Module The module or component that generated the data.
 * @param Sensor The sensor or data source.
 * @param Value The value or data point to be logged.
 * @param final An optional boolean flag indicating whether this is the final
 *              data point.
 *
 * The function uses a mutex (`InfluxClientMutex`) to control access to the shared
 * resource (Influx client). If the mutex cannot be acquired within a specified
 * timeout (`HighRateMutexWaitTicks`), the data point is stored in an alternate
 * buffer (`fast_datapoints`) to be added to the Influx client later.
 *
 * If `SerialDebugMode` and `HighRateDetailDebugging` are defined, debug messages
 * are printed to the serial monitor.
 *
 * @note If there is an error writing the Point to the transmission buffer,
 *       the `writeError` flag is set.
 *
 * @return void
 */
void logDataInflux(unsigned long long uS, unsigned long long S, String Module, String Sensor, String Value, bool final = false) {
  static Point* datapoint;
  if (!datapoint)  // No point exists
  {
    datapoint = new Point(Module);
    datapoint->addTag("device", DEVICE);
    datapoint->setTime(S * 1000000LL + uS);
  }

  datapoint->addField(Sensor, Value);

  if (final)  // All data added to datapoint
  {
    // Store Influx Data
    // Try to take the mutex but don't wait for long
    if (xSemaphoreTake(InfluxClientMutex, (TickType_t)HighRateMutexWaitTicks) == pdTRUE) {  // Send Point to Transmission Buffer
    #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      Serial.println("Highrate Take Mutex");
    #endif

      writeError = writeError || client.writePoint(*datapoint);

      fastPointCount++;

      xSemaphoreGive(InfluxClientMutex);  // After accessing the shared resource give the mutex and allow other processes to access it

      delete datapoint;  // Deallocate memory
    } else {             // We could not obtain the semaphore and can therefore not access the shared resource safely.
      // Send Point to alternate buffer
      #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      Serial.println("Highrate during early alt store");
      #endif

      if(fastPointCountAlt < BATCH_SIZE)
      {
        fast_datapoints[fastPointCountAlt] = datapoint;  // Transfer allocated memory

        fastPointCountAlt++;
      }
      else
      {
        #ifdef SerialDebugMode
        Serial.println("Alternate Fast Point Buffer Full");
        #endif

        delete datapoint; // Deallocate memory and lose data
        writeError = true;
      }

      #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      Serial.println("Highrate during alt store");
      #endif
    }  // end mutex take


#if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
  Serial.println("Highrate Return Mutex");
#endif

    datapoint = nullptr;  // Reset pointer for next measurement
  }
}
#endif  // InfluxLogging


#ifdef SDLogging
/**
 * @brief Logs data to an SD card file.
 *
 * This function logs a data point to a file on the SD card. The file is opened
 * or created if it doesn't exist, and the data is appended to the file.
 *
 * @param uS The microsecond part of the timestamp.
 * @param S The second part of the timestamp.
 * @param Module The module or component that generated the data.
 * @param Sensor The sensor or data source.
 * @param Value The value or data point to be logged.
 * @param final An optional boolean flag indicating whether this is the final
 *              data point. If true, the file is flushed and closed after writing
 *              the data point.
 *
 * The data is logged in the following format:
 * "DEVICE - Time: S.uS - Module: Sensor - Value"
 *
 * @note If the file cannot be opened or created, an error message is printed to
 *       the serial monitor if `SerialDebugMode` is defined.
 *
 * @return void
 */
void logDataSD(unsigned long long uS, unsigned long long S, String Module, String Sensor, String Value, bool final = false)
{  
  // Open the file if it's not already open
  if (!dataLog)
    dataLog = SD.open(FILENAME, FILE_APPEND);

  if (dataLog) {
    String data = String(DEVICE) + " - Time: " + String(S) + "S " + String(uS) + "uS - " + Module + ": " + Sensor + " - " + Value;
    dataLog.println(data);

#ifdef SerialDebugMode
    Serial.println("Data written successfully");
#endif

    // If this is the final data point, flush and close the file
    if (final) {
      dataLog.flush();
      dataLog.close();
    }
  } else {
#ifdef SerialDebugMode
    Serial.println("Error opening file for logging data point.");
#endif
  }
}
#endif  // SD Logging

/**
 * @brief Log a string data point to the specified logging destinations.
 *
 * @param uS Timestamp in microseconds.
 * @param S Timestamp in seconds.
 * @param Module The module name.
 * @param Sensor The sensor name.
 * @param Value The string data value to be logged.
 * @param final Indicates whether this is the final data point in a sequence.
 *
 * @return void
 */
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, String Value, bool final = false) {
#ifdef InfluxLogging
  logDataInflux(uS, S, Module, Sensor, Value, final);
#endif

#ifdef SDLogging
#ifdef SDRedundantLoggingOnly
if (writeError)
{
#endif
  logDataSD(uS, S, Module, Sensor, Value, final);
#ifdef SDRedundantLoggingOnly
writeError = false;
}
#endif
#endif
}

/**
 * @brief Log a float data point to the specified logging destinations.
 *
 * @param uS Timestamp in microseconds.
 * @param S Timestamp in seconds.
 * @param Module The module name.
 * @param Sensor The sensor name.
 * @param Value The float data value to be logged.
 * @param final Indicates whether this is the final data point in a sequence.
 *
 * @return void
 */
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, float Value, bool final = false) {
  char buffer[20];                // Adjust buffer size as needed
  dtostrf(Value, 12, 6, buffer);  // Adjust precision as needed (currently 6 decimal places)
  logDataPoint(uS, S, Module, Sensor, String(buffer), final);
}

/**
 * @brief Log a double data point to the specified logging destinations.
 *
 * @param uS Timestamp in microseconds.
 * @param S Timestamp in seconds.
 * @param Module The module name.
 * @param Sensor The sensor name.
 * @param Value The double data value to be logged.
 * @param final Indicates whether this is the final data point in a sequence.
 *
 * @return void
 */
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, double Value, bool final = false) {
  char buffer[20];                // Adjust buffer size as needed
  dtostrf(Value, 12, 6, buffer);  // Adjust precision as needed (currently 6 decimal places)
  logDataPoint(uS, S, Module, Sensor, String(buffer), final);
}

/**
 * @brief Log a int data point to the specified logging destinations.
 *
 * @param uS Timestamp in microseconds.
 * @param S Timestamp in seconds.
 * @param Module The module name.
 * @param Sensor The sensor name.
 * @param Value The int data value to be logged.
 * @param final Indicates whether this is the final data point in a sequence.
 *
 * @return void
 */
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, int Value, bool final = false) {
  logDataPoint(uS, S, Module, Sensor, String(Value), final);
}

/**
 * @brief Log a long data point to the specified logging destinations.
 *
 * @param uS Timestamp in microseconds.
 * @param S Timestamp in seconds.
 * @param Module The module name.
 * @param Sensor The sensor name.
 * @param Value The long data value to be logged.
 * @param final Indicates whether this is the final data point in a sequence.
 *
 * @return void
 */
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, long Value, bool final = false) {
  logDataPoint(uS, S, Module, Sensor, String(Value), final);
}

/**
 * @brief Configures the ISM330DHCX sensor.
 *
 * This function initializes the ISM330DHCX sensor by setting up the accelerometer
 * and gyroscope ranges, data rates, and interrupt configurations. It also checks
 * for the presence of the sensor and prints debug information to the serial
 * monitor if the `SerialDebugMode` macro is defined.
 *
 * If the sensor is not found, the function enters an infinite loop.
 *
 * @note This function is based on the Arduino example code for the ISM330DHCX sensor.
 *
 * @return void
 */
void setIsm330Config() {  // From Arduino Example
  // check for ism330dhcx
  if (!ism330dhcx.begin_I2C()) {
#ifdef SerialDebugMode
    Serial.println("Failed to find ISM330DHCX chip");
#endif
    while (1) {
      delay(10);
    }
  }

  ism330dhcx.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
#ifdef SerialDebugMode
  Serial.print("Accelerometer range set to: ");
  Serial.println(ism330dhcx.getAccelRange());
  switch (ism330dhcx.getAccelRange()) {
    case LSM6DS_ACCEL_RANGE_2_G:
      Serial.println("+-2G");
      break;
    case LSM6DS_ACCEL_RANGE_4_G:
      Serial.println("+-4G");
      break;
    case LSM6DS_ACCEL_RANGE_8_G:
      Serial.println("+-8G");
      break;
    case LSM6DS_ACCEL_RANGE_16_G:
      Serial.println("+-16G");
      break;
  }
#endif

  ism330dhcx.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
#ifdef SerialDebugMode
  Serial.print("Gyro range set to: ");
  Serial.println(ism330dhcx.getGyroRange());
  switch (ism330dhcx.getGyroRange()) {
    case LSM6DS_GYRO_RANGE_125_DPS:
      Serial.println("125 degrees/s");
      break;
    case LSM6DS_GYRO_RANGE_250_DPS:
      Serial.println("250 degrees/s");
      break;
    case LSM6DS_GYRO_RANGE_500_DPS:
      Serial.println("500 degrees/s");
      break;
    case LSM6DS_GYRO_RANGE_1000_DPS:
      Serial.println("1000 degrees/s");
      break;
    case LSM6DS_GYRO_RANGE_2000_DPS:
      Serial.println("2000 degrees/s");
      break;
    case ISM330DHCX_GYRO_RANGE_4000_DPS:
      Serial.println("4000 degrees/s");
      break;
  }
#endif

  ism330dhcx.setAccelDataRate(LSM6DS_RATE_833_HZ);
#ifdef SerialDebugMode
  Serial.print("Accelerometer data rate set to: ");
  switch (ism330dhcx.getAccelDataRate()) {
    case LSM6DS_RATE_SHUTDOWN:
      Serial.println("0 Hz");
      break;
    case LSM6DS_RATE_12_5_HZ:
      Serial.println("12.5 Hz");
      break;
    case LSM6DS_RATE_26_HZ:
      Serial.println("26 Hz");
      break;
    case LSM6DS_RATE_52_HZ:
      Serial.println("52 Hz");
      break;
    case LSM6DS_RATE_104_HZ:
      Serial.println("104 Hz");
      break;
    case LSM6DS_RATE_208_HZ:
      Serial.println("208 Hz");
      break;
    case LSM6DS_RATE_416_HZ:
      Serial.println("416 Hz");
      break;
    case LSM6DS_RATE_833_HZ:
      Serial.println("833 Hz");
      break;
    case LSM6DS_RATE_1_66K_HZ:
      Serial.println("1.66 KHz");
      break;
    case LSM6DS_RATE_3_33K_HZ:
      Serial.println("3.33 KHz");
      break;
    case LSM6DS_RATE_6_66K_HZ:
      Serial.println("6.66 KHz");
      break;
  }
#endif

  ism330dhcx.setGyroDataRate(LSM6DS_RATE_833_HZ);
#ifdef SerialDebugMode
  Serial.print("Gyro data rate set to: ");
  switch (ism330dhcx.getGyroDataRate()) {
    case LSM6DS_RATE_SHUTDOWN:
      Serial.println("0 Hz");
      break;
    case LSM6DS_RATE_12_5_HZ:
      Serial.println("12.5 Hz");
      break;
    case LSM6DS_RATE_26_HZ:
      Serial.println("26 Hz");
      break;
    case LSM6DS_RATE_52_HZ:
      Serial.println("52 Hz");
      break;
    case LSM6DS_RATE_104_HZ:
      Serial.println("104 Hz");
      break;
    case LSM6DS_RATE_208_HZ:
      Serial.println("208 Hz");
      break;
    case LSM6DS_RATE_416_HZ:
      Serial.println("416 Hz");
      break;
    case LSM6DS_RATE_833_HZ:
      Serial.println("833 Hz");
      break;
    case LSM6DS_RATE_1_66K_HZ:
      Serial.println("1.66 KHz");
      break;
    case LSM6DS_RATE_3_33K_HZ:
      Serial.println("3.33 KHz");
      break;
    case LSM6DS_RATE_6_66K_HZ:
      Serial.println("6.66 KHz");
      break;
  }
#endif

  ism330dhcx.configInt1(false, false, true);  // accelerometer DRDY on INT1
  ism330dhcx.configInt2(false, true, false);  // gyro DRDY on INT2
}

/**
 * @brief Handles the MQTT connection establishment.
 *
 * This function is called when the MQTT connection is established. It performs
 * the following tasks:
 *
 * 1. Enables debugging messages for the MQTT client.
 * 2. Sets the Last Will and Testament message for the MQTT client.
 * 3. Publishes a "Device connected" message to the subscribe topic.
 * 4. Subscribes to the publish topic and sets up a callback function to handle
 *    incoming messages.
 * 5. Prints a "Connected to MQTT broker" message if `SerialDebugMode` is defined.
 *
 * The callback function handles the following incoming messages:
 *
 * - `NODE_RED_RESET`: Resets the device by calling `ESP.restart()`.
 * - `NODE_RED_STOP`: Stops the recording by setting `ISM330DHCX_Run` and `RSSI_Run`
 *   to `false`. If `InfluxLogging` is defined, it also transmits the Influx buffer.
 * - `NODE_RED_START`: Starts the recording by setting `ISM330DHCX_Run` and `RSSI_Run`
 *   to `true`.
 *
 * If `SerialDebugMode` is defined, it prints debug messages for received messages.
 *
 * @note This function was contributed by Andrew Bryan.
 *
 * @return void
 */
void onConnectionEstablished() {  // Contributed by Andrew Bryan
  mqttClient.enableDebuggingMessages();
  mqttClient.enableLastWillMessage(MQTT_TOPIC_SUBCRIBE, "Device disconnected.");
  mqttClient.publish(MQTT_TOPIC_SUBCRIBE, "Device connected.", 0);
  mqttClient.subscribe(
    MQTT_TOPIC_PUBLISH, [](const String& payload) {
#ifdef SerialDebugMode
      Serial.print("Message arrived on topic: ");
      Serial.print(MQTT_TOPIC_PUBLISH);
      Serial.print(". Message: ");
      Serial.println(payload);
#endif

      if (payload == NODE_RED_RESET) {
        Serial.println("Resetting device");
        ESP.restart();
      } else if (payload == NODE_RED_STOP) {
        ISM330DHCX_Run = false;
        RSSI_Run = false;
#ifdef InfluxLogging
        transmitInfluxBuffer();
#endif

#ifdef SerialDebugMode
        Serial.println("Recording stopped");
#endif
      } else if (payload == NODE_RED_START) {
        ISM330DHCX_Run = true;
        RSSI_Run = true;

#ifdef SerialDebugMode
        Serial.println("Recording started");
#endif
      }
    },
    0);
#ifdef SerialDebugMode
  Serial.println("Connected to MQTT broker.");
#endif
}

#endif  // FunctionsCode
