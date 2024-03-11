// Functions.cpp
// Contains function prototypes and helper functions for everything else
// Written By: Jordan Kooyman

#ifndef FunctionsCode
#define FunctionsCode


#include "Configuration.h"

#ifdef InfluxLogging

void setInfluxConfig() {
    // Enable batching and timestamp precision
    client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::US).batchSize(BATCH_SIZE));
}
#endif


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
  while (WiFi.status() != WL_CONNECTED && cycles < (2*WIFICONNECTTIME)) {
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
  if (cycles >= (2*WIFICONNECTTIME))
  {
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


void setWifiMultiConfig()
{
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


void setTime()
{
  #ifdef HasNeopixel
  pixel.setPixelColor(0, pixel.Color(255, 0, 255));
  pixel.show();
  #endif
  setenv("TZ", TimeZoneOffset,1);

  bool useGPSTime = true;

  if(!checkGPSTime())
  {
    // Network 1
    #ifdef HasNeopixel
    pixel.setPixelColor(0, pixel.Color(0, 0, 125));
    pixel.show();
    #endif
    // Connect to WiFi for Initial Setup (Wait until Connected)
    bool connected = setWifiConfig();

    if (!tryTimeSync(connected)) // Attempt NTP Time Sync on primary network
    {
      // Network 2
      #ifdef HasNeopixel
      pixel.setPixelColor(0, pixel.Color(0, 125, 125));
      pixel.show();
      #endif
      WiFi.disconnect(true, true);
      delay(250);
      connected = setWifiConfig(2); // Use alternate network to get time from
      useGPSTime = !tryTimeSync(connected);
    }
    else
      useGPSTime = false; // First time sync attempt worked
  }
  if(useGPSTime)
  {
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


// Returns False if attempts ran out or otherwise failed, True if time successfully synchronized
bool tryTimeSync(bool WiFi_Connected)
{
  if (!WiFi_Connected)
    return false;
  int attemptCount = 1;
  struct timeval tv;
  // Update Stored System Time
  gettimeofday(&tv, nullptr);
  #ifdef SerialDebugMode
  Serial.print("Time: "); Serial.println(getSeconds());
  #endif
  #ifdef OLEDDebugging
  display.print("Time: "); display.println(getSeconds());
  display.display();
  #endif

  while(getSeconds() < 1000)
  { // Retry Set Initial System Epoch Time from Internet
    #ifdef SerialDebugMode
    Serial.print("Attempt "); Serial.println(attemptCount);
    #endif
    #ifdef OLEDDebugging
    display.print("Attempt "); display.println(attemptCount);
    display.display();
    #endif
    timeSync(TimeZoneOffset, ntpServer, ntpServer2); // Influx Client Function
    // Update Stored System Time
    gettimeofday(&tv, nullptr);
    attemptCount++;
    if (attemptCount > I_WIFI_ATTEMPT_COUNT_LIMIT)
      return false;
  }
  return true;
}


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
      return true; // GPS has current time
    }
  }

  // No valid time found
  return false;
}


int32_t getGPSTime()
{
  // Ensure GPS data is available
  if (!GPSSerial.available()) {
    return 0; // Return 0 if no data available
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
      break; // Exit loop once time is obtained
    }
  }

  return gpsTime;
}


int setUnixtime(int32_t unixtime) 
{ // From https://github.com/espressif/arduino-esp32/issues/1444
  timeval epoch = {unixtime, 0};
  return settimeofday((const timeval*)&epoch, 0);
}


// GPS PPS Interrupt Handler
// Designed to be as simple (and fast) as possible to maintain the high accuracy of the PPS pulse
// IRAM_ATTR keeps this function in RAM, for faster response times
// ARDUINO_ISR_ATTR keeps the function loaded for optimal ISR use
void ARDUINO_ISR_ATTR GPS_PPS_ISR()
{// inspired by the approach used here: https://forum.arduino.cc/t/super-accurate-1ms-yr-gps-corrected-rtc-clock-without-internet-ntp/640518
  // incrementer on time of day to maintain high precision (higher than NTP and GPS NMEA sentences)
  // look at current time and compare to last interrupt time, then
  // (just?) get current time, round it to the nearest 100 microseconds, then set it again
  // or zero current microseconds and set seconds to last globally stored value (+1?)
  // tv.tv_usec = PPSOffsetMicroseconds;
  // #ifndef NTPDelayHigh
  // tv.tv_sec++; // This may skew the time by 1 second, depending on NTP accuracy
  // #endif
  // // settimeofday(&tv, nullptr);
  GPS_us = micros();
  GPSSync = true;

  #ifdef SerialDebugMode
  #ifdef InterruptDebugging
  Serial.println(); Serial.println(); Serial.println(); Serial.println();
  Serial.print("GPS PPS Reset");
  Serial.println(); Serial.println();
  #endif
  #endif
}


// Function that gets current epoch/unix time for InfluxDB Timestamping
// From: https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/
unsigned long long getTime() { // unsigned long long type to match the type used by Point setTime function in InfluxDbClient
  time_t now; // https://cplusplus.com/reference/ctime/time_t/
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    #ifdef SerialDebugMode
    Serial.println("Failed to obtain time");
    #endif
    #ifdef OLEDDebugging
    display.println("Failed to obtain time");
    display.display();
    #endif
    return(0);
  }
  time(&now);
  return now;
}


// Get Seconds since Epoch
unsigned long long getSeconds()
{
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}


// Get Microseconds
unsigned long long getuSeconds()
{
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_usec;
}


// Transmit Buffer
#ifdef InfluxLogging
void transmitInfluxBuffer()
{
  #if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.print(xPortGetCoreID());
    Serial.print(" Core - Transmit Buffer");
  #endif
  // Transmit Buffered Datapoints
  if(xSemaphoreTake(InfluxClientMutex, ( TickType_t ) 1000) == pdTRUE)
  {

  #if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.print("Transmit take Mutex");
  #endif

  // Force Influx Client buffer to send
  client.flushBuffer();

  #if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.print("Transmit Done");
  #endif
  xSemaphoreGive(InfluxClientMutex); // After accessing the shared resource give the mutex and allow other processes to access it
  }
  fastPointCount = 0;
  slowPointCount = 0;

  #if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.print("Transmit Return Mutex");
    Serial.print(" Aux Buffer Size: ");
    Serial.print(fastPointCountAlt);
  #endif

  for (int i = 0; i < fastPointCountAlt; i++) 
  { // Buffer Queued Fast Rate Datapoints
    writeError = writeError || client.writePoint(*fast_datapoints[i]); // Dereference pointer to get the Point object
  }

  #if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.print("Queued Buffer");
  #endif

  fastPointCountAlt = 0;
  // Iterate through the array and delete each element
  for (int i = 0; i < BATCH_SIZE; i++)
  {
    if (fast_datapoints[i] != nullptr)
    { // Check if the element is not already deleted
      delete fast_datapoints[i]; // Delete the element
      fast_datapoints[i] = nullptr; // Set the pointer to nullptr
    }
  }


  #if defined(SerialDebugMode) && defined(TransmitDetailDebugging)
    Serial.println("Emptied Buffer, Done");
  #endif
}
#endif


#ifdef SDLogging
// void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, String Value, bool final)
// {
//   if (dataLog)
//     dataLog = SD.open(FILENAME(DEVICE), FILE_WRITE);

//   // #ifdef SerialDebugMode
//   // Serial.print("Data Logging - ");
//   // Serial.println(dataLog);
//   // #endif

//   dataLog.print("Time: "); dataLog.print(S);
//   dataLog.print(" "); dataLog.print(uS); dataLog.print(" - ");
//   dataLog.print(Sensor); dataLog.print(": "); dataLog.println(Value);

//   if(final)
//   {
//     dataLog.flush();
//     dataLog.close();
//   }
// }

void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, String Value, bool final = false)
{
  // Open the file if it's not already open
  if (!dataLog)
    dataLog = SD.open(FILENAME(DEVICE), FILE_APPEND);

  if (dataLog) {
    String data = "Time: " + String(S) + "S " + String(uS) + "uS - " + Sensor + Value;
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

void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, float Value, bool final = false) {
  char buffer[20]; // Adjust buffer size as needed
  dtostrf(Value, 12, 6, buffer); // Adjust precision as needed (currently 6 decimal places)
  logDataPoint(uS, S, Sensor, String(buffer), final);
}

void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, double Value, bool final = false) {
  char buffer[20]; // Adjust buffer size as needed
  dtostrf(Value, 12, 6, buffer); // Adjust precision as needed (currently 6 decimal places)
  logDataPoint(uS, S, Sensor, String(buffer), final);
}

void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, int Value, bool final = false) {
  logDataPoint(uS, S, Sensor, String(Value), final);
}

void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, long Value, bool final = false) {
  logDataPoint(uS, S, Sensor, String(Value), final);
}
#endif


void setIsm330Config() { // From Arduino Example
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

    ism330dhcx.configInt1(false, false, true); // accelerometer DRDY on INT1
    ism330dhcx.configInt2(false, true, false); // gyro DRDY on INT2
}

void setMqttConfig()
{
  Serial.print("Connecting to: ");
  Serial.print(MQTT_SERVER);
  Serial.print(":");
  Serial.println(MQTT_PORT);
  Serial.print("Topic: ");
  Serial.println(MQTT_TOPIC_PUBLISH);
}

void onConnectionEstablished()
{
  mqttClient.enableDebuggingMessages();
  mqttClient.enableLastWillMessage(MQTT_TOPIC_SUBCRIBE, "Device disconnected.");
  mqttClient.publish(MQTT_TOPIC_SUBCRIBE, "Device connected.", 0);
  mqttClient.subscribe(MQTT_TOPIC_PUBLISH, [](const String & payload)
  {
    Serial.print("Message arrived on topic: ");
    Serial.print(MQTT_TOPIC_PUBLISH);
    Serial.print(". Message: ");
    Serial.println(payload);

    if (payload == NODE_RED_RESET)
    {
      Serial.println("Resetting device");
      ESP.restart();
    }
    else if (payload == NODE_RED_STOP)
    {
      ISM330DHCX_Run = false;
      RSSI_Run = false;
      transmitInfluxBuffer();
      slowPointCount = 0;
      fastPointCount = 0;
      fastPointCountAlt = 0;

      #ifdef SerialDebugMode
      Serial.println("Recording stopped");
      #endif
    }
      else if (payload == NODE_RED_START)
    {
      ISM330DHCX_Run = true;
      RSSI_Run = true;

      #ifdef SerialDebugMode
      Serial.println("Recording started");
      #endif
    }
  }, 0);
  Serial.println("Connected to MQTT broker.");
}

#endif // FunctionsCode
