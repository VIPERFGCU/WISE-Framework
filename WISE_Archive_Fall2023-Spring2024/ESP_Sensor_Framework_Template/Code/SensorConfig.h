/**
 * @file SensorConfig.h
 * @brief Sensor variables, setup, and configurations.
 * @author Jordan Kooyman
 *
 * This file contains the configuration and setup code for various sensors used
 * in the program. It defines sensor-specific variables, constants, and settings
 * required for proper operation.
 *
 * The actual implementation of the sensor data polling is located in the
 * SensorsFast.cpp or SensorsSlow.cpp files, depending on the sensor's data rate.
 * The function prototypes for these implementations should be added to the
 * Prototypes.h file.
 *
 * Timers are started on Core0 (low-rate) and Core1 (high-rate) to initiate
 * continuous polling of the sensors. The associated core also handles the data
 * processing for the respective sensors.
 *
 * @note This file should be updated with the appropriate sensor configurations
 *       and settings for the specific project.
 */


// Sensor Timer Configurations
// Sum of all sensor runs per second must be less than 20,000
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#_CPPv423esp_timer_create_args_t

#define SkipUnhandledInterruptsFast true
#define SkipUnhandledInterruptsSlow true


// High-rate Sensors
/*****************************************************************************/
// FastSensorExample
// bool FastSensorExample_Run = true;
// esp_timer_handle_t FastSensorExample_Timer;
// #define FastSensorExample_RunsPerSecond 100
// #define FastSensorExample_Pin 4
// #define FastSensorExample_Name "Fast Sensor Example"
// const esp_timer_create_args_t FastSensorExample_Config = {.callback = &FastSensorExample_Callback, .name = FastSensorExample_Name, .skip_unhandled_events = SkipUnhandledInterruptsFast};

// ISM330DHCX Accelerometer
Adafruit_ISM330DHCX ism330dhcx;
bool ISM330DHCX_Run = true;
esp_timer_handle_t ISM330DHCX_Timer;
#define ISM330DHCX_RunsPerSecond 100
#define ISM330DHCX_Name "Onboard Gyro/Accelerometer"
const esp_timer_create_args_t ISM330DHCX_Config = { .callback = &ISM330DHCX_Callback, .name = ISM330DHCX_Name, .skip_unhandled_events = SkipUnhandledInterruptsFast };


// Low-rate Sensors
/*****************************************************************************/
// SlowSensorExample
// bool SlowSensorExample_Run = true;
// unsigned long long SlowSensorExample_Time = 0;
// #define SlowSensorExample_SecondsPerRun 5
// #define SlowSensorExample_Pin 3
// #define SlowSensorExample_Name "Slow Sensor Example"

// WiFi Strength (RSSI)
bool RSSI_Run = true;
unsigned long long RSSI_Time = 0;
#define RSSI_SecondsPerRun 2
#define RSSI_Pin 3
#define RSSI_Name "RSSI"