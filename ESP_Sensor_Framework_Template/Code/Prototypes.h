// Prototypes.h
// Function Prototypes
// Written By: Jordan Kooyman

// ESP_GPS_Timers_Config_Example_Reference.ino
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