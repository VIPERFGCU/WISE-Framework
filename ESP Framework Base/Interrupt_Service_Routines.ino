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
// IRAM_ATTR keeps this function in RAM, for faster response times
void IRAM_ATTR GPS_PPS_ISR()
{
  esp_timer_stop(Sensor0Timer);

    // GPS Timing Update task

  esp_timer_start_periodic(Sensor0Timer, 1000000 / Sensor0RunsPerSecond);
}