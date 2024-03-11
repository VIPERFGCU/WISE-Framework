// SensorsSlow.cpp
// Low-rate Sensor Polling Functions
// Not using esp timers to keep Core 1 free for high-rate tasks
// Written By: Jordan Kooyman

#include "esp_timer.h"

#ifndef SensorsSlowCode
#define SensorsSlowCode

// Start Virtual/Software Timers for Sensors
void startLowRateSensors()
{
  // SlowSensorExample_Time = getSeconds() + SlowSensorExample_SecondsPerRun;
  RSSI_Time = getSeconds() + RSSI_SecondsPerRun;
}

// Check Virtual/Software Timers for Sensors
void checkLowRateSensors()
{ // Low Rate Software Timer Checking

  // if (SlowSensorExample_Time >= getSeconds())
  //   SlowSensorExample_Poll();

  if (RSSI_Time <= getSeconds() && RSSI_Run)
    RSSI_Poll();
}

// SlowSensorExample Polling Function
  // void SlowSensorExample_Poll()
  // {
  //   // Create local datapoint
  //   #ifdef InfluxLogging
  //   Point datapoint(SlowSensorExample_Name);
  //   datapoint.addTag("device", DEVICE);

    // // Attatch Timestamp to Data
    // datapoint.setTime(WritePrecision::S);
    // #endif
    // #ifdef SDLogging
    // unsigned long long timestampS = getSeconds();
    // unsigned long long timestampuS = getuSeconds();
    // #endif

  //   // Poll and Process Sensor Data
  //   int data = digitalRead(SlowSensorExample_Pin)
  //   #ifdef InfluxLogging
  //   datapoint.addField("Example Slow Value", data);
  //   #endif

    // // Send Point to Transmission Buffer
    // #ifdef InfluxLogging
    // writeError = writeError || client.writePoint(datapoint);
    // #endif
    // #ifdef SDLogging
    // logDataPoint(timestampuS, timestampS, "Example Slow Value", data);
    // #endif

  //   slowPointCount++;
  //   SlowSensorExample_Time = getSeconds() + SlowSensorExample_SecondsPerRun;
  // }

// Wifi Strength Polling Function
  void RSSI_Poll()
  {
    // Create local datapoint
    #ifdef InfluxLogging
    Point datapoint(RSSI_Name);
    datapoint.addTag("device", DEVICE);

    // Attatch Timestamp to Data
    datapoint.setTime(WritePrecision::S);
    #endif
    #ifdef SDLogging
    unsigned long long timestampS = getSeconds();
    unsigned long long timestampuS = getuSeconds();
    #endif
    // Report RSSI of currently connected network
    int RSSI = WiFi.RSSI();


    #ifdef InfluxLogging
    datapoint.addField("rssi", RSSI);
    #endif

    #ifdef SDLogging
    logDataPoint(timestampuS, timestampS, "RSSI", RSSI);
    #endif

    // Send Point to Transmission Buffer
    #ifdef InfluxLogging
    writeError = writeError || client.writePoint(datapoint);
    #endif

    slowPointCount++;
    RSSI_Time = getSeconds() + RSSI_SecondsPerRun;

    #ifdef SerialDebugMode
    Serial.println("RSSI Poll");
    #endif
  }


#endif // SensorsSlowCode