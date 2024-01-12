// Core0.cpp
// Low rate sensor polling and data transmission handling framework
// No Interrupts on this Core
// Written by: Jordan Kooyman

#ifndef Core0Code
#define Core0Code

void core0Setup()
{
  // Set Initial System Epoch Time from Internet
  configTime(0, 0, ntpServer);

  // Connect to InfluxDB and Mosquito/NodeRed
  setInfluxConfig();

  // Setup Attached Sensors
  setIsm330Config();

  // Start Timers
  startLowRateSensors();

  // Debugging Serial Printouts
  #ifdef SerialDebugMode
  Serial.begin(115200);
  #endif
}

void core0Loop()
{
  // Start Transmitting Data if total amount is nearing target Batch Size
  if ((slowPointCount + fastPointCount) > (BATCH_SIZE * (StartTransmissionPercentage / 100)))
  {
    transmitInfluxBuffer();
  }

  // Client Write Error Handling
  if (writeError)
  {
    writeError = false;

    #ifdef SerialDebugMode
    Serial.println("Write Error Occured");
    #endif
  }

  // Low Rate Software Timer Checking
  // if (SlowSensorExample_Time >= getSeconds())
  //   SlowSensorExample_Poll();
}

#endif Core0Code