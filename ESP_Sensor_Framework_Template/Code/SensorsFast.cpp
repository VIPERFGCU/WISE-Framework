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

// FastSensorExample_Timer 'ISR' Callback Function
    // static void FastSensorExample_Callback(void* args)
    // {
    //   if(!FastSensorExample_Run) // Remote disable last-ditch check
    //     return;
    //
    //   // Create local datapoint
    //   Point datapoint(FastSensorExample_Name);
    //   datapoint.addTag("device", DEVICE);

    //   // Attatch Timestamp to Data
    //   datapoint.setTime(WritePrecision::US);

    //   // Poll & Process Sensor Data
    //   datapoint.addField("Example Fast Value", digitalRead(FastSensorExample_Pin));

      // #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      //   Serial.println("Highrate before store");
      // #endif

      // // Store Data
      // // Try to take the mutex but don't wait for long
      // if(xSemaphoreTake(InfluxClientMutex, ( TickType_t ) HighRateMutexWaitTicks) == pdTRUE) 
      // { // Send Point to Transmission Buffer
      //   #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      //     Serial.println("Highrate Take Mutex");
      //   #endif

      //   writeError = writeError || client.writePoint(datapoint);

      //   fastPointCount++;

      //   xSemaphoreGive(InfluxClientMutex); // After accessing the shared resource give the mutex and allow other processes to access it
      // }
      // else
      // { // We could not obtain the semaphore and can therefore not access the shared resource safely.
      //   // Send Point to alternate buffer
      //   #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      //     Serial.println("Highrate during early  alt store");
      //   #endif

      //   fast_datapoints[fastPointCountAlt] = new Point(datapoint);//datapoint;// Core 0 Panic (Store Prohibited) Occurs here

      //   fastPointCountAlt++;

      //   #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      //     Serial.println("Highrate during alt store");
      //   #endif
      // } // mutex take

      // #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
      //   Serial.println("Highrate Return Mutex");
      // #endif
    // } // End FastSensorExample_Callback


// ISM330DHCX_Timer 'ISR' Callback Function
    static void ISM330DHCX_Callback(void* args)
    {
      if (!ISM330DHCX_Run) // Remote disable last-ditch check
        return;

      // Create local datapoint
      Point datapoint(ISM330DHCX_Name);
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

      #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
        Serial.println("Highrate before store");
      #endif

      // Store Data
      // Try to take the mutex but don't wait for long
      if(xSemaphoreTake(InfluxClientMutex, ( TickType_t ) HighRateMutexWaitTicks) == pdTRUE) 
      { // Send Point to Transmission Buffer
        #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
          Serial.println("Highrate Take Mutex");
        #endif

        writeError = writeError || client.writePoint(datapoint);

        fastPointCount++;

        xSemaphoreGive(InfluxClientMutex); // After accessing the shared resource give the mutex and allow other processes to access it
      }
      else
      { // We could not obtain the semaphore and can therefore not access the shared resource safely.
        // Send Point to alternate buffer
        #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
          Serial.println("Highrate during early  alt store");
        #endif

        fast_datapoints[fastPointCountAlt] = new Point(datapoint);//datapoint;// Core 0 Panic (Store Prohibited) Occurs here

        fastPointCountAlt++;

        #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
          Serial.println("Highrate during alt store");
        #endif
      } // mutex take

      #if defined(SerialDebugMode) && defined(HighRateDetailDebugging)
        Serial.println("Highrate Return Mutex");
      #endif
    } // End ISM330DHCX_Callback
  

#endif // SensorsFastCode