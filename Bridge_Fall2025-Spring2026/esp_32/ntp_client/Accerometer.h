/* Author: Siang Chin
 * Desc: This file standardized the acceleroiter data with a timestamp.
 * Call create_timestamped_accel_data to create a timestamp of the acceleration data in nanoseconds
 * It uses ntp_client's getEpochTime() to get the time
 * All acceceration is scaled so 9.8m/s is 1.0. This is done to account for the
 * many different sensors present.( Ex: Some may report an integer, while other libraries may report a float. )
 * Each sensor has 2 functions:
 *  void get_accelerometer_data(float data[3])
 *  void init_accelerometer()
 */

#pragma once
#include "ntp_client.h"
// ================ Accelorometer Choice. Only enable 1 at a time ============
#define USE_ISM 0
#define USE_LSM 1
// ================= ISM SENSOR CONFIG =====================
#if USE_ISM
// ---------------- SENSOR CALIBRATIION ---------------
const float ACCEL_X_OFFSET = 0.11 - 0.007;
const float ACCEL_X_SCALE = 0.101684;
const float ACCEL_Y_OFFSET = -0.17 - 0.17;
const float ACCEL_Y_SCALE = 0.101511;
const float ACCEL_Z_OFFSET = 0.05;
const float ACCEL_Z_SCALE = 0.102654;
// ---------------------------------------------------

#include <Adafruit_ISM330DHCX.h>
Adafruit_ISM330DHCX ism330dhcx;
void get_accelerometer_data(float data[3]) {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  ism330dhcx.getEvent(&accel, &gyro, &temp);

  data[0] = accel.acceleration.x;
  data[1] = accel.acceleration.y;
  data[2] = accel.acceleration.z;
}


void init_accelerometer() {
  if(!ism330dhcx.begin_I2C()) {
    Serial.println("Failed to find ism330dhcx accelemeter");
    delay(100);  
  };
  ism330dhcx.setAccelDataRate(LSM6DS_RATE_104_HZ);
  ism330dhcx.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
}
#endif
// ===================================================
// ===================== LSM SENSOR CONFIG =================
#if USE_LSM

// ---------------- SENSOR CALIBRATIION ---------------
//const float ACCEL_X_OFFSET = 0.17;
//const float ACCEL_X_SCALE = 0.101808;
//const float ACCEL_Y_OFFSET = -0.15;
//const float ACCEL_Y_SCALE = 0.103007;
//const float ACCEL_Z_OFFSET = 0.07;
//const float ACCEL_Z_SCALE = 0.101463;
const float ACCEL_X_OFFSET = 0.04;
const float ACCEL_X_SCALE = 0.205407;
const float ACCEL_Y_OFFSET = 0.04;
const float ACCEL_Y_SCALE = 0.203206;
const float ACCEL_Z_OFFSET = 0.03;
const float ACCEL_Z_SCALE = 0.203442;
// ---------------------------------------------------

#include <Adafruit_LSM6DSOX.h>
Adafruit_LSM6DSOX sox;
void get_accelerometer_data(float data[3]) {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  sox.getEvent(&accel, &gyro, &temp);

  data[0] = accel.acceleration.x;
  data[1] = accel.acceleration.y;
  data[2] = accel.acceleration.z;
}

void init_accelerometer() {
  if (!sox.begin_I2C()) {
    // if (!sox.begin_SPI(LSM_CS)) {
    // if (!sox.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) {
     Serial.println("Failed to find LSM6DSOX chip");
    while (1) {
      delay(10);
    }
  }
  
  sox.setAccelDataRate(LSM6DS_RATE_104_HZ);
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
}
#endif

// =========================================
// ======== Process Data =================

struct TimeStampedAccelData {
  uint64_t timestamp;  // Unix time in seconds
  float ax; // X acceleration
  float ay; // Y acceleration
  float az; // Z acceleration
};

void create_timestamped_accel_data(TimeStampedAccelData &data_struct, const float &raw_x, const float &raw_y,const float &raw_z) {
  data_struct.timestamp = getEpochTime() * 1000ULL; // convert microseconds to nanoseconds
  data_struct.ax = (raw_x - ACCEL_X_OFFSET) * ACCEL_X_SCALE;
  data_struct.ay = (raw_y - ACCEL_Y_OFFSET) * ACCEL_Y_SCALE;
  data_struct.az = (raw_z - ACCEL_Z_OFFSET) * ACCEL_Z_SCALE;
}
// ====================================
