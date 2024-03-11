// Prototypes.h
// Function Prototypes
// Written By: Jordan Kooyman

// ESP_GPS_Timers_Config_Example_Reference.ino
void setup();
void loop();

// SensorsFast.cpp
void startHighRateSensors();
// static void FastSensorExample_Callback(void* args);
static void ISM330DHCX_Callback(void* args);

// SensorsSlow.cpp
void startLowRateSensors();
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
#endif
#ifdef SDLogging
void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, String Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, float Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, double Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, int Value, bool final);
void logDataPoint(unsigned long long uS, unsigned long long S, String Sensor, long Value, bool final);
#endif
void setIsm330Config();
void setMqttConfig();
void onConnectionEstablished();