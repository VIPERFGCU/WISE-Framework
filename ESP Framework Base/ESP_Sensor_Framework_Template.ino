// ESP_Sensor_Framework_Template.ino  --  V0.5
// Framework for running a high-rate hardware timer based sensor polling scheme on core 1 and a low rate
// sensor polling scheme on core 0, with data transmission handled by core 0 and time accuracy updates 
// using GPS handled by core 1
// Written by: Jordan Kooyman

#include "esp_timer.h"
//#include <TinyGPSPlus.h>
#include <WiFiMulti.h>
#include "time.h"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Parametric Configurations
#define NTPDelayHigh // Uncomment to increment second on GPS PPS Interrupt
#define PPSOffsetMicroseconds 0
#define ssid "fgcu-campus"
#define password ""
#define GPSBaudRate 4800
#define ntpServer "pool.ntp.org"
#define INFLUXDB_URL "http://69.88.163.33:8086"
#define INFLUXDB_TOKEN "cq8t98XqeCl49-CMZ2M7Ci1i7h1DHp6buu1nbOeaPxU7mgU8_Z3OorfS6J3fRlBMqCnYqTTt7mqUJ93rfq3Mfg=="
#define INFLUXDB_ORG "867116c343b9f084"
#define INFLUXDB_BUCKET "test"

// Pin Definitions
#define GPS_PPS_Pin 13

//#define GPSSerial Serial1


// Global Variables:
TaskHandle_t Task1;
TaskHandle_t Task2;
//TinyGPSPlus GPS;
WiFiMulti wifiMulti;


// Function Prototypes:
// Core0.ino
void core0Setup();
void core0Loop();
void core0OS(void * pvParameters);

// Core1.ino
void core1Setup();
void core1Loop();
void core1OS(void * pvParameters);

// Interrupt_Service_Routines.ino
static void Sensor0_Callback(void* args);
void IRAM_ATTR GPS_PPS_ISR();


// Function that gets current epoch/unix time for InfluxDB Timestamping
// From: https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/
unsigned long long getTime() { // unsigned long long type to match the type used by Point setTime function in InfluxDbClient
  time_t now; // https://cplusplus.com/reference/ctime/time_t/
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}


void setup()
{  //create a task that will be executed in the core0OS() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    core0OS,   /* Task function. */
                    "Core 0",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the core1OS() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    core1OS,   /* Task function. */
                    "Core 1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}

void core0OS(void * pvParameters)
{
  //core0Setup();
  while(true)
  {
    //core0Loop();
    digitalRead(0);
  }
}

void core1OS(void * pvParameters)
{
  //core1Setup();
  while(true)
  {
    //core1Loop();
    digitalRead(1);
  }
}

void loop()
{
    // Do Nothing!
}

