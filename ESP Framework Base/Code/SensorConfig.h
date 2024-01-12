// SensorConfig.h
// Sensor Variables, Setup, and Configurations
// Implementation (data polling) in SensorsFast.cpp or SensorsSlow.cpp files
// Add implementation function prototype to Prototypes.h
// Timers are started in Core0 (low-rate) and Core1 (high-rate) to begin continuous polling
// Associated Core also handles data processing
// Written By: Jordan Kooyman


// Sensor Timer Configurations
  // Sum of all sensor runs per second must be less than 20,000
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#_CPPv423esp_timer_create_args_t

#define SkipUnhandledInterruptsFast true
#define SkipUnhandledInterruptsSlow true


// High-rate Sensors
/*****************************************************************************/
// FastSensorExample
  // esp_timer_handle_t FastSensorExample_Timer;
  // #define FastSensorExample_RunsPerSecond 100
  // #define FastSensorExample_Pin 4
  // #define FastSensorExample_Name "Fast Sensor Example"
  // const esp_timer_create_args_t FastSensorExample_Config = {.callback = &FastSensorExample_Callback, .name = FastSensorExample_Name, .skip_unhandled_events = SkipUnhandledInterruptsFast}; 

// Gyro
  Adafruit_ISM330DHCX ism330dhcx;
  esp_timer_handle_t Gyro_Timer;
  #define Gyro_RunsPerSecond 100
  #define Gyro_Name "Onboard Gyro/Accelerometer"
  const esp_timer_create_args_t Gyro_Config = {.callback = &Gyro_Callback, .name = Gyro_Name, .skip_unhandled_events = SkipUnhandledInterruptsFast};


// Low-rate Sensors
/*****************************************************************************/
// SlowSensorExample
  // unsigned long long SlowSensorExample_Time = 0;
  // #define SlowSensorExample_SecondsPerRun 5
  // #define SlowSensorExample_Pin 3
  // #define SlowSensorExample_Name "Slow Sensor Example"