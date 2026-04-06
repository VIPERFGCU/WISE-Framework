#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Accelerometer.h"

// ==== Interface to main.cpp ====
extern void send_mesh_packet(const char *json_payload);

// Variables provided by other files
extern char device_id[32]; 
extern const int MS_INTERVAL;

// ==== Functions ====
void transmit_accelerometer_data(const TimeStampedAccelData &accel_data);
void setup_mqtt();

#endif // MQTT_CLIENT_H