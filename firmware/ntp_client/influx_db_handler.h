#pragma once
#ifdef USE_OLD  // Disable the influx_db_handler
#include <WiFi.h>
#include <HTTPClient.h>

#include "Accerometer.h"
// ================== InfluxDB settings ===============
const char* server = "http://10.100.100.1:8086";
const char* org = "c5916b31b0278fcc";
const char* bucket = "sensors";
const char* token = "Lny9EC-LMo535uBb0WpNfBwt7Rv9Vz8v5sfwCKKjxbjXoQ5TBe920Oac1JVljmPaEuQiTbFgDj_ZZMkt5VfNeQ==";
const int SENSOR_ID = 4;  // Change this per sensor( Manual for now )

void transmit_accelerometer_data(const TimeStampedAccelData &accel_data) {
  String line = "acceleration_fl,sensor=esp32_" + String(SENSOR_ID) + " ";
  line += "x=" + String(accel_data.ax, 6) + ",";
  line += "y=" + String(accel_data.ay, 6) + ",";
  line += "z=" + String(accel_data.az, 6);
  line += " " + String(accel_data.timestamp);

  Serial.println("Line: " + line);

  HTTPClient http;
  String url = String(server) + "/api/v2/write?org=" + org +
               "&bucket=" + bucket + "&precision=us"; // match microseconds
  http.begin(url);
  http.addHeader("Authorization", "Token " + String(token));
  http.addHeader("Content-Type", "text/plain");

  int httpResponseCode = http.POST(line);
  Serial.println("HTTP Response code: " + String(httpResponseCode));

  http.end();
}
#endif
