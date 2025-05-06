/**
 * @file Prototypes.h
 * @brief Function prototypes for the entire program.
 * @authors Jordan Kooyman and Andrew Bryan
 *
 * This file contains the function prototypes for various functions used throughout
 * the program. These prototypes declare the function signatures, allowing other
 * parts of the program to call these functions without needing to know their
 * implementation details.
 *
 * The prototypes in this file likely correspond to the function implementations
 * found in other files, such as Functions.cpp or SensorsFast.cpp.
 */

// ESP_Sensor_Framework_Template.ino
void setup();
void loop();

// SensorsFast.cpp
void startHighRateSensors();
void stopHighRateSensors();
// static void FastSensorExample_Callback(void* args);
static void ISM330DHCX_Callback(void* args);

// SensorsSlow.cpp
void startLowRateSensors();
void stopLowRateSensors();
void checkLowRateSensors();
// void SlowSensorExample_Poll();
void RSSI_Poll();

// Functions.cpp
#ifdef InfluxLogging
void setInfluxConfig();
#endif
bool setWifiConfig(int Network = 1);
void setWifiMultiConfig();
bool tryTimeSync(bool WiFi_Connected);
bool checkGPSTime();
int32_t getGPSTime();
int setUnixtime(int32_t unixtime);
void ARDUINO_ISR_ATTR GPS_PPS_ISR();
unsigned long long getTime();
unsigned long long getSeconds();

unsigned long long getuSeconds();
#ifdef InfluxLogging
void transmitInfluxBuffer();
void logDataInflux(unsigned long long uS, unsigned long long S, String Module, String Sensor, String Value, bool final);
#endif
#ifdef SDLogging
void logDataSD(unsigned long long uS, unsigned long long S, String Module, String Sensor, String Value, bool final);
#endif
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, String Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, float Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, double Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, int Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Module, String Sensor, long Value, bool final);
void setIsm330Config();
void onConnectionEstablished();