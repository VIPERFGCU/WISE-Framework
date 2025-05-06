/**
 * @file SensorsFast.cpp
 * @brief High-rate sensor polling functions.
 * @author Jordan Kooyman
 *
 * This file contains the implementation of high-rate sensor polling functions.
 * These functions are responsible for reading data from sensors that require
 * frequent sampling, such as accelerometers, gyroscopes, or other fast-changing
 * data sources.
 *
 * The functions defined in this file are likely called by a high-priority
 * hardware timer interrupt or a similar mechanism to ensure timely and
 * accurate data acquisition, and therefore should be kept as minimal as possible.
 */

#ifndef SensorsFastCode
#define SensorsFastCode

// Sensor Timer Start
void startHighRateSensors() {
  // ESP Timer Configurations
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html

  // FastSensorExample
  // esp_timer_create(&FastSensorExample_Config, &FastSensorExample_Timer);
  // esp_timer_start_periodic(FastSensorExample_Timer, (1000000 / FastSensorExample_RunsPerSecond));

  // ISM330DHCX
  esp_timer_create(&ISM330DHCX_Config, &ISM330DHCX_Timer);
  esp_timer_start_periodic(ISM330DHCX_Timer, (1000000 / ISM330DHCX_RunsPerSecond));
}

void stopHighRateSensors() {
  // Todo
}

/**
 * @brief Example callback function for a fast sensor timer interrupt.
 *
 * This function serves as a template or example for implementing a callback
 * function that is called periodically by a timer interrupt. It demonstrates
 * how to read data from a fast sensor (e.g., a digital input pin) and log it
 * to the appropriate data storage.
 *
 * The function performs the following tasks:
 *
 * 1. Checks if the `FastSensorExample_Run` flag is set. If not, it returns
 *    without performing any further actions.
 * 2. Retrieves the current timestamp in microseconds and seconds using the
 *    `getuSeconds()` and `getSeconds()` functions, respectively.
 * 3. Reads the sensor data by calling `digitalRead(FastSensorExample_Pin)`,
 *    which reads the state of the digital input pin specified by
 *    `FastSensorExample_Pin`.
 * 4. Logs the sensor data to the appropriate data storage using the
 *    `logDataPoint()` function, passing the timestamp, sensor name
 *    (`FastSensorExample_Name`), data value, and a flag indicating that this
 *    is the final data point for this measurement.
 *
 * The `logDataPoint()` function is assumed to handle the data storage and
 * formatting according to the desired output (e.g., SD card file or Influx
 * database).
 *
 * This function is intended to serve as a template or example for implementing
 * similar callback functions for other fast sensors or data sources.
 *
 * @note This function is only a generic template and is commented out.
 *
 * @param args A pointer to additional arguments that may be passed to the
 *             callback function. In this case, it is not used.
 */
// FastSensorExample_Timer 'ISR' Callback Function
// static void FastSensorExample_Callback(void* args)
// {
//   if(!FastSensorExample_Run) // Remote disable last-ditch check
//     return;

// unsigned long long timestampuS = getuSeconds();
// unsigned long long timestampS = getSeconds();


//   // Poll & Process Sensor Data
//   int data = digitalRead(FastSensorExample_Pin)

// logDataPoint(timestampuS, timestampS, FastSensorExample_Name, "Example Fast Value", data, true);
// } // End FastSensorExample_Callback


/**
 * @brief Callback function for ISM330DHCX_Timer ISR.
 * 
 * This function polls sensor data from ISM330DHCX sensor, retrieves timestamps, and logs the data points.
 * 
 * @param args Pointer to arguments passed to the callback function.
 * @return void
 */
static void ISM330DHCX_Callback(void* args) {
  if (!ISM330DHCX_Run)  // Remote disable last-ditch check
    return;

  unsigned long long timestampuS = getuSeconds();
  unsigned long long timestampS = getSeconds();

  // Poll Sensor Data
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;

  ism330dhcx.getEvent(&accel, &gyro, &temp);

  logDataPoint(timestampuS, timestampS, ISM330DHCX_Name, "Gyro X", gyro.gyro.x);
  logDataPoint(timestampuS, timestampS, ISM330DHCX_Name, "Gyro Y", gyro.gyro.y);
  logDataPoint(timestampuS, timestampS, ISM330DHCX_Name, "Gyro Z", gyro.gyro.z);
  logDataPoint(timestampuS, timestampS, ISM330DHCX_Name, "Accel X", accel.acceleration.x);
  logDataPoint(timestampuS, timestampS, ISM330DHCX_Name, "Accel Y", accel.acceleration.y);
  logDataPoint(timestampuS, timestampS, ISM330DHCX_Name, "Accel Z", accel.acceleration.z, true);
}  // End ISM330DHCX_Callback


#endif  // SensorsFastCode