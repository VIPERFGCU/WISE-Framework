// Configuration.h
// Parametric Global Configuration constants used throughout entire program
// Written by: Jordan Kooyman

#ifndef ConfigCode
#define ConfigCode

// Parametric Configurations
/*****************************************************************************/
// #define SerialDebugMode
// #define TransmitDetailDebugging
// #define HighRateDetailDebugging
#define SerialBaudRate 115200


// Device
#define DEVICE "ESP32" // Update This! <-------------------------------------------------------------------------------------
#define GPS_PPS_PIN 27 // Pulse-Per-Second Pin used for GPS Time Synchronization (Check)
#define PPSOffsetMicroseconds 0
#define GPSSerial Serial1
#define GPSBaudRate 115200

// WiFi Network(s) for Setup/Initialization - Any additions or subtractions made here also need to be made to setWifiConfig() in Functions.cpp
#define I_WIFI_SSID "fgcu-campus" // Update This! <--------------------------------------------------------------------------
#define I_WIFI_PASSWORD "" // Update This! <---------------------------------------------------------------------------------
#define I_WIFI_SSID2 "" // Alternate network for time sync only // Update This! <--------------------------------------------
#define I_WIFI_PASSWORD2 "" // Update This! <--------------------------------------------------------------------------------
#define I_WIFI_ATTEMPT_COUNT_LIMIT 2
// WiFi Network(s) for Operation - Any additions or subtractions made here also need to be made to setWifiMultiConfig() in Functions.cpp
#define O_WIFI_SSID I_WIFI_SSID
#define O_WIFI_PASSWORD I_WIFI_PASSWORD
// #define O_WIFI_SSID2 ""
// #define O_WIFI_PASSWORD2 ""

// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://69.88.163.33:8086" // Update This! <------------------------------------------------------------
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "478mGdfNVYM9gbTEn7X6kSPsy659RJo1zu_Znc0jW-VWsekCP4URT11zZSrV_0XRzhURcux58EQD9wbP_Azfpw==" // Update This! <----------------
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "867116c343b9f084" // Update This! <--------------------------------------------------------------------
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "test" // Update This! <-----------------------------------------------------------------------------

// Transmission Batching Controls
#define BATCH_SIZE 100
#define StartTransmissionPercentage 60
#define HighRateMutexWaitTicks 10

// Time
#define TimeZoneOffset "EST+5EDT,M3.2.0/2,M11.1.0/2" // Check This! <--------------------------------------------------------
#define ntpServer "pool.ntp.org"
#define ntpServer2 "time.nis.gov"


// Global Variables:
/*****************************************************************************/
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point* fast_datapoints[BATCH_SIZE];
SemaphoreHandle_t InfluxClientMutex = NULL;
bool writeError = false;
int slowPointCount = 0;
int fastPointCount = 0;
int fastPointCountAlt = 0;
struct timeval tv;
TaskHandle_t Task1;
TaskHandle_t Task2;
TinyGPSPlus gps;
WiFiMulti wifiMulti;

#endif // ConfigCode