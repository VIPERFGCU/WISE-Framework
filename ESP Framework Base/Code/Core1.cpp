// Core1.cpp
// High rate sensor polling framework and timing accuracy verification using GPS
// Utilizing ESP Timer library to allow for multiple high-rate polling calls at once
// All Interrupts on this Core
// Written by: Jordan Kooyman

#ifndef Core1Code
#define Core1Code

#include "esp_timer.h"




void core1Setup()
{
  // Setup GPS Module
  attachInterrupt(GPS_PPS_PIN, GPS_PPS_ISR, RISING);
  //GPSSerial.begin(GPSBaudRate);
  
  // Setup Attached Sensors

  // Start Timers
  startHighRateSensors();
}

void core1Loop()
{
  // Update Stored System Time for GPS Interrupt Usage
  gettimeofday(&tv, nullptr);


}

#endif // Core1Code