// Functions.cpp
// Contains function prototypes and helper functions for everything else
// Written By: Jordan Kooyman

#ifndef FunctionsCode
#define FunctionsCode

void setInfluxConfig() {
    // Enable batching and timestamp precision
    client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::US).batchSize(BATCH_SIZE));
}

void setWifiConfig() {
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to wifi");
    while (wifiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
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
  // Transmit Buffered Datapoints
  HighRateSensorLockout = true;

  client.flushBuffer();
  
  HighRateSensorLockout = false;
  fastPointCount = 0;
  slowPointCount = 0;

  // Buffer Queued Fast Rate Datapoints
  for (int i = 0; i < fastPointCountAlt; i++) {
    // Buffer Queued Datapoint
    writeError = writeError || client.writePoint(* fast_datapoints[i]);

    // Clear Queued Datapoint
    fast_datapoints[i]->clearFields();
    fast_datapoints[i]->clearTags();
  }
}


void setIsm330Config() {
    // check for ism330dhcx
    if (!ism330dhcx.begin_I2C()) {
        Serial.println("Failed to find ISM330DHCX chip");
        while (1) {
            delay(10);
        }
    }

    ism330dhcx.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
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

    ism330dhcx.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
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

    ism330dhcx.setAccelDataRate(LSM6DS_RATE_833_HZ);
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

    ism330dhcx.setGyroDataRate(LSM6DS_RATE_833_HZ);
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

    ism330dhcx.configInt1(false, false, true); // accelerometer DRDY on INT1
    ism330dhcx.configInt2(false, true, false); // gyro DRDY on INT2
}


#endif // FunctionsCode