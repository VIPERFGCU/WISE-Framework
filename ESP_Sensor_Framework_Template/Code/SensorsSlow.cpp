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

void stopLowRateSensors()
{
 // Todo
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
  //   unsigned long long timestampS = getSeconds();
  //   unsigned long long timestampuS = getuSeconds();

  //   // Poll and Process Sensor Data
  //   int data = digitalRead(SlowSensorExample_Pin)

  //   logDataPoint(timestampuS, timestampS, SlowSensorExample_Name, "Example Slow Value", data, true);

  //   slowPointCount++;
  //   SlowSensorExample_Time = getSeconds() + SlowSensorExample_SecondsPerRun;
  // }

// Wifi Strength Polling Function
  void RSSI_Poll()
  {
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


#endif // SensorsSlowCode