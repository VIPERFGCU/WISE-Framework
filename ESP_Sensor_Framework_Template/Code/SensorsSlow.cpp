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
  //   Point datapoint(SlowSensorExample_Name);
  //   datapoint.addTag("device", DEVICE);

  //   // Attatch Timestamp to Data
  //   datapoint.setTime(WritePrecision::S);

  //   // Poll and Process Sensor Data
  //   datapoint.addField("Example Slow Value", digitalRead(SlowSensorExample_Pin));

  //   // Send Point to Transmission Buffer
  //   writeError = writeError || client.writePoint(datapoint);

  //   slowPointCount++;
  //   SlowSensorExample_Time = getSeconds() + SlowSensorExample_SecondsPerRun;
  // }

// Wifi Strength Polling Function
  void RSSI_Poll()
  {
    // Create local datapoint
    Point datapoint(RSSI_Name);
    datapoint.addTag("device", DEVICE);

    // Attatch Timestamp to Data
    datapoint.setTime(WritePrecision::S);

    // Report RSSI of currently connected network
    datapoint.addField("rssi", WiFi.RSSI());

    // Send Point to Transmission Buffer
    writeError = writeError || client.writePoint(datapoint);

    slowPointCount++;
    RSSI_Time = getSeconds() + RSSI_SecondsPerRun;

    #ifdef SerialDebugMode
    Serial.println("RSSI Poll");
    #endif
  }


#endif // SensorsSlowCode