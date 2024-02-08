// Functions.cpp
// Contains function prototypes and helper functions for everything else
// Written By: Jordan Kooyman

#ifndef FunctionsCode
#define FunctionsCode

void setInfluxConfig() {
    // Enable batching and timestamp precision
    client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::US).batchSize(BATCH_SIZE));
}


void setWifiConfig(int Network) {
    WiFi.mode(WIFI_STA);
    if (Network == 1)
      WiFi.begin(I_WIFI_SSID, I_WIFI_PASSWORD);
    else if (Network == 2)
      WiFi.begin(I_WIFI_SSID2, I_WIFI_PASSWORD2);

    #ifdef SerialDebugMode
    Serial.print("Connecting to wifi");
    Serial.print(WiFi.SSID());
    #endif
    while (WiFi.status() != WL_CONNECTED) {
        #ifdef SerialDebugMode
        Serial.print(".");
        #endif
        delay(500);
    }
    #ifdef SerialDebugMode
    Serial.print("Connected to wifi");
    Serial.println(WiFi.SSID());
    #endif
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
    while (wifiMulti.run() != WL_CONNECTED) {
        #ifdef SerialDebugMode
        Serial.print(".");
        #endif
        delay(500);
    }
    #ifdef SerialDebugMode
    Serial.print("Connected to wifi");
    Serial.println(WiFi.SSID());
    #endif
}


void setTime()
{
  bool useGPSTime;
  if (!tryTimeSync()) // Attempt NTP Time Sync on primary network
  {
    WiFi.disconnect(true, true);
    delay(250);
    setWifiConfig(2); // Use alternate network to get time from
    useGPSTime = !tryTimeSync();
  }

  if(useGPSTime)
  {
    #ifdef SerialDebugMode
    Serial.println("Using GPS Time");
    #endif
    setUnixtime(getGPSTime());
  }

  gettimeofday(&tv, nullptr);
}


// Returns False if attempts ran out, True if time successfully synchronized
bool tryTimeSync()
{
  int attemptCount = 1;

  // Update Stored System Time
  gettimeofday(&tv, nullptr);
  #ifdef SerialDebugMode
  Serial.print("Time: "); Serial.println(getSeconds());
  #endif

  while(getSeconds() < 1000)
  { // Retry Set Initial System Epoch Time from Internet
    #ifdef SerialDebugMode
    Serial.print("Attempt "); Serial.println(attemptCount);
    #endif
    timeSync(TimeZoneOffset, ntpServer, ntpServer2);
    // Update Stored System Time
    gettimeofday(&tv, nullptr);
    attemptCount++;
    if (attemptCount > I_WIFI_ATTEMPT_COUNT_LIMIT)
      return false;
  }
  return true;
}


int32_t getGPSTime()
{ // From https://github.com/espressif/arduino-esp32/issues/1444
  time_t t_of_day; 
  struct tm t;   
  while (GPSSerial.available()) {
    gps.encode(char(GPSSerial.read()));
    if (gps.date.isUpdated())
    {
      t.tm_year = gps.date.year()-1900;
      t.tm_mon = gps.date.month()-1;           // Month, 0 - jan
      t.tm_mday = gps.date.day();          // Day of the month
      t.tm_hour = gps.time.hour();
      t.tm_min =  gps.time.minute();
      t.tm_sec = gps.time.second();
      t_of_day = mktime(&t);
  
      return t_of_day;
    }
  }
}


int setUnixtime(int32_t unixtime) 
{ // From https://github.com/espressif/arduino-esp32/issues/1444
  timeval epoch = {unixtime, 0};
  return settimeofday((const timeval*)&epoch, 0);
}


// GPS PPS Interrupt Handler
// Designed to be as simple (and fast) as possible to maintain the high accuracy of the PPS pulse
// IRAM_ATTR keeps this function in RAM, for faster response times
void IRAM_ATTR GPS_PPS_ISR()
{// inspired by the approach used here: https://forum.arduino.cc/t/super-accurate-1ms-yr-gps-corrected-rtc-clock-without-internet-ntp/640518
  // incrementer on time of day to maintain high precision (higher than NTP and GPS NMEA sentences)
  // look at current time and compare to last interrupt time, then
  // (just?) get current time, round it to the nearest 100 microseconds, then set it again
  // or zero current microseconds and set seconds to last globally stored value (+1?)
  tv.tv_usec = PPSOffsetMicroseconds;
  #ifndef NTPDelayHigh
  tv.tv_sec++; // This may skew the time by 1 second, depending on NTP accuracy
  #endif
  settimeofday(&tv, nullptr);

  #ifdef SerialDebugMode
  Serial.println(); Serial.println(); Serial.println(); Serial.println();
  Serial.print("GPS PPS Reset - ");
  Serial.println(tv.tv_sec);
  Serial.println(); Serial.println();
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
    return(0);
  }
  time(&now);
  return now;
}


// Get Seconds since Epoch
unsigned long long getSeconds()
{
  return tv.tv_sec;
}


// Transmit Buffer
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


void setIsm330Config() {
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


#endif // FunctionsCode