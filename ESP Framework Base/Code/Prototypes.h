// Prototypes.h
// Function Prototypes
// Written By: Jordan Kooyman

// ESP_GPS_Timers_Config_Example_Reference.ino
void setup();
void loop(); // Unused, but required
void core0OS(void * pvParameters);
void core1OS(void * pvParameters);

// SensorsFast.cpp
// static void FastSensorExample_Callback(void* args);
static void Gyro_Callback(void* args);

// SensorsSlow.cpp
// void SlowSensorExample_Poll();

// Functions.cpp
void setInfluxConfig();
void setWifiConfig();
void IRAM_ATTR GPS_PPS_ISR();
unsigned long long getTime();
unsigned long long getSeconds();
void setIsm330Config();

// Core0.cpp
void core0Setup();
void core0Loop();

// Core1.cpp
void core1Setup();
void core1Loop();