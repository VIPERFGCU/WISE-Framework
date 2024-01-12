// SensorsSlow.cpp
// Low-rate Sensor Polling Functions
// Written By: Jordan Kooyman

#include "esp_timer.h"

#ifndef SensorsSlowCode
#define SensorsSlowCode

// Start Virtual/Software Timers for Sensors
void startLowRateSensors()
{
  // SlowSensorExample_Time = getSeconds() + SlowSensorExample_SecondsPerRun;
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


#endif // SensorsSlowCode