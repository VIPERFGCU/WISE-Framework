// Interrupt_Service_Routines.ino
// Framework for all ISR functions, intended to be run on Core1 as needed
// Note: Only time-critical tasks should be run here,
//   use flags back to standard runtime whereever possible
// Written by: Jordan Kooyman

#include "esp_timer.h"

// Sensor0Timer 'ISR' Callback Function
static void Sensor0_Callback(void* args)
{
  // Perform time-sensitive sensor polling, store data for further processing later

  timer0Flag = true;
}

// GPS PPS Interrupt Handler
// Designed to be as simple (and fast) as possible to maintain the high accuracy of the PPS pulse
// IRAM_ATTR keeps this function in RAM, for faster response times
void IRAM_ATTR GPS_PPS_ISR()
{// inspired by the approach used here: https://forum.arduino.cc/t/super-accurate-1ms-yr-gps-corrected-rtc-clock-without-internet-ntp/640518
  // incrementer on time of day to maintain high precision (higher than NTP and GPS NMEA sentences)
  // look at current time and compare to last interrupt time, then
  // (just?) get current time, round it to the nearest 100 microseconds, then set it again
  // or zero current microseconds and set seconds to last globally stored value (+1?)
  tv.tv_usec = PPSOffsetMicroseconds;
  #ifndef NTPDelayHigh
  tv.tv_sec++; // This may skew the time by 1 second, depending on NTP accuracy
  #endif
  settimeofday(&tv, nullptr);
}