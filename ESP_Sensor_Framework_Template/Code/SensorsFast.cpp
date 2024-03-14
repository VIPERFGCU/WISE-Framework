// SensorsFast.cpp
// High-rate Sensor Polling Functions
// Written By: Jordan Kooyman

#ifndef SensorsFastCode
#define SensorsFastCode

// Sensor Timer Start
void startHighRateSensors()
{
  // ESP Timer Configurations
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html

  // FastSensorExample
    // esp_timer_create(&FastSensorExample_Config, &FastSensorExample_Timer);
    // esp_timer_start_periodic(FastSensorExample_Timer, (1000000 / FastSensorExample_RunsPerSecond));

  // ISM330DHCX
  esp_timer_create(&ISM330DHCX_Config, &ISM330DHCX_Timer);
  esp_timer_start_periodic(ISM330DHCX_Timer, (1000000 / ISM330DHCX_RunsPerSecond));
}

void stopHighRateSensors()
{
 // Todo
}

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


// ISM330DHCX_Timer 'ISR' Callback Function
    static void ISM330DHCX_Callback(void* args)
    {
      if (!ISM330DHCX_Run) // Remote disable last-ditch check
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
    } // End ISM330DHCX_Callback
  

#endif // SensorsFastCode