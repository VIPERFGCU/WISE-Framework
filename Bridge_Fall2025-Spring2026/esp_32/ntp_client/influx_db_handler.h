#pragma once
#include <WiFi.h>
#include <HTTPClient.h>

#include "Accerometer.h"
// ================== InfluxDB settings ===============
const char* server = "http://10.65.168.250:8086";
const char* org = "myorg";
const char* bucket = "testbucket";
const char* token = "simpletoken";
const int SENSOR_ID = 3;  // Change this per sensor( Manual for now )

void transmit_accelerometer_data(const TimeStampedAccelData &accel_data) {
  // Format InfluxDB Line Protocol
  String line = "acceleration_fl,sensor=esp32_" + String(SENSOR_ID) + " ";
  line += "x=" + String(accel_data.ax) + ",";
  line += "y=" + String(accel_data.ay) + ",";
  line += "z=" + String(accel_data.az);
  line += " " + String(accel_data.timestamp);  // Timestamp in seconds

  Serial.print("Line: ");
  Serial.print(line);
  Serial.print("\n");
  // Send data
  HTTPClient http;
  String url = String(server) + "/api/v2/write?org=" + org + "&bucket=" + bucket + "&precision=ns";
  http.begin(url);
  http.addHeader("Authorization", "Token " + String(token));
  http.addHeader("Content-Type", "text/plain");

  int httpResponseCode = http.POST(line); // send the data

  Serial.print("HTTP Response code: ");
  Serial.print(httpResponseCode);
  Serial.print("\n");
  http.end();
}
