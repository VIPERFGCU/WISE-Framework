/**
 * @file SensorsSlow.cpp
 * @brief Low-rate sensor polling functions.
 * @author Jordan Kooyman
 *
 * This file contains the implementation of low-rate sensor polling functions.
 * These functions are responsible for reading data from sensors that do not
 * require frequent sampling, such as environmental sensors or other slowly
 * changing data sources.
 *
 * The functions defined in this file are not using ESP timers to keep Core 1
 * free for high-rate tasks. Instead, they are called periodically by a
 * software timer.
 */

#include "esp_timer.h"

#ifndef SensorsSlowCode
#define SensorsSlowCode

// Start Virtual/Software Timers for Sensors
void startLowRateSensors() {
  // SlowSensorExample_Time = getSeconds() + SlowSensorExample_SecondsPerRun;
  RSSI_Time = getSeconds() + RSSI_SecondsPerRun;
}

void stopLowRateSensors() {
  // Todo
}

// Check Virtual/Software Timers for Sensors
void checkLowRateSensors() {  // Low Rate Software Timer Checking

  // if (SlowSensorExample_Time >= getSeconds())
  //   SlowSensorExample_Poll();

  if (RSSI_Time <= getSeconds() && RSSI_Run)
    RSSI_Poll();
}

/**
 * @brief Example polling function for a slow sensor.
 *
 * This function serves as a template or example for implementing a polling
 * function that reads data from a slow sensor (e.g., a digital input pin) and
 * logs it to the appropriate data storage.
 *
 * The function performs the following tasks:
 *
 * 1. Retrieves the current timestamp in microseconds and seconds using the
 *    `getuSeconds()` and `getSeconds()` functions, respectively.
 * 2. Reads the sensor data by calling `digitalRead(SlowSensorExample_Pin)`,
 *    which reads the state of the digital input pin specified by
 *    `SlowSensorExample_Pin`.
 * 3. Logs the sensor data to the appropriate data storage using the
 *    `logDataPoint()` function, passing the timestamp, sensor name
 *    (`SlowSensorExample_Name`), data value, and a flag indicating that this
 *    is the final data point for this measurement.
 * 4. Increments the `slowPointCount` variable, which likely tracks the number
 *    of data points logged for the slow sensor.
 * 5. Updates the `SlowSensorExample_Time` variable with the next time the
 *    sensor should be polled, based on the `SlowSensorExample_SecondsPerRun`
 *    constant, which specifies the interval between sensor readings.
 *
 * The `logDataPoint()` function is assumed to handle the data storage and
 * formatting according to the desired output (e.g., SD card file or Influx
 * database).
 *
 * This function is intended to serve as a template or example for implementing
 * similar polling functions for other slow sensors or data sources.
 *
 * @note This function is only a generic template and is commented out.
 */
// void SlowSensorExample_Poll()
// {
//   unsigned long long timestampS = getSeconds();
//   unsigned long long timestampuS = getuSeconds();

//   // Poll and Process Sensor Data
//   int data = digitalRead(SlowSensorExample_Pin)

//   logDataPoint(timestampuS, timestampS, SlowSensorExample_Name, "Example Slow Value", data, true);

//   slowPointCount++;
//   SlowSensorExample_Time = getSeconds() + SlowSensorExample_SecondsPerRun;
// }

// Wifi Strength Polling Function
void RSSI_Poll() {
  unsigned long long timestampS = getSeconds();
  unsigned long long timestampuS = getuSeconds();

  // Report RSSI of currently connected network
  int RSSI = WiFi.RSSI();

  logDataPoint(timestampuS, timestampS, RSSI_Name, "RSSI", RSSI, true);

  slowPointCount++;
  RSSI_Time = getSeconds() + RSSI_SecondsPerRun;

#ifdef SerialDebugMode
  Serial.println("RSSI Poll");
#endif
}


#endif  // SensorsSlowCode