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
void setInfluxConfig();
void setWifiConfig(int Network = 1);
void setWifiMultiConfig();
bool tryTimeSync();
int32_t getGPSTime();
int setUnixtime(int32_t unixtime);
void IRAM_ATTR GPS_PPS_ISR();
unsigned long long getTime();
unsigned long long getSeconds();
void setIsm330Config();