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
    // esp_timer_start_periodic(FastSensorExample_Timer, 1000000 / FastSensorExample_RunsPerSecond);

  // Gyro
    esp_timer_create(&Gyro_Config, &Gyro_Timer);
    esp_timer_start_periodic(Gyro_Timer, 1000000 / Gyro_RunsPerSecond);
}

// FastSensorExample_Timer 'ISR' Callback Function
    // static void FastSensorExample_Callback(void* args)
    // {
    //   // Create local datapoint
    //   Point datapoint(FastSensorExample_Name);
    //   datapoint.addTag("device", DEVICE);

    //   // Attatch Timestamp to Data
    //   datapoint.setTime(WritePrecision::US);

    //   // Poll & Process Sensor Data
    //   datapoint.addField("Example Fast Value", digitalRead(FastSensorExample_Pin));

    //   // Store Data
    //   if (HighRateSensorLockout)
    //   {
    //     // Send Point to alternate buffer
    //     *fast_datapoints[fastPointCountAlt] = datapoint;

    //     fastPointCountAlt++;
    //   }
    //   else
    //   {
    //     // Send Point to Transmission Buffer
    //     writeError = writeError || client.writePoint(datapoint);

    //     fastPointCount++;
    //   }
    // }


// Gyro_Timer 'ISR' Callback Function
    static void Gyro_Callback(void* args)
    {
      // Create local datapoint
      Point datapoint(Gyro_Name);
      datapoint.addTag("device", DEVICE);

      // Attatch Timestamp to Data
      datapoint.setTime(WritePrecision::US);

      // Poll Sensor Data
      sensors_event_t accel;
      sensors_event_t gyro;
      sensors_event_t temp;

      ism330dhcx.getEvent(&accel, &gyro, &temp);

      datapoint.addField("Gyro X", gyro.gyro.x);
      datapoint.addField("Gyro Y", gyro.gyro.y);
      datapoint.addField("Gyro Z", gyro.gyro.z);
      datapoint.addField("Accel X", accel.acceleration.x);
      datapoint.addField("Accel Y", accel.acceleration.y);
      datapoint.addField("Accel Z", accel.acceleration.z);

      // Store Data
      if (HighRateSensorLockout)
      {
        // Send Point to alternate buffer
        *fast_datapoints[fastPointCountAlt] = datapoint;

        fastPointCountAlt++;
      }
      else
      {
        // Send Point to Transmission Buffer
        writeError = writeError || client.writePoint(datapoint);

        fastPointCount++;
      }
    }
  

#endif // SensorsFastCode