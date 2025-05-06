/**
 * @file Configuration.h
 * @brief Parametric Global Configuration constants used throughout the entire program.
 * @details Written by Jordan Kooyman and Andrew Bryan.
 */

#ifndef ConfigCode
#define ConfigCode

// Parametric Configurations
/*****************************************************************************/
// #define SerialDebugMode
// #define TransmitDetailDebugging
// #define HighRateDetailDebugging
// #define InterruptDebugging
#define SerialBaudRate 115200
// #define OLEDDebugging // Not fully implemented, Untested
#define HasNeopixel

// Data Logging Modes
#define InfluxLogging
#define SDLogging

// #define SDRedundantLoggingOnly


// Device
#define DEVICE "ESP32-X"  // Update This! <-------------------------------------------------------------------------------------
#define GPS_PPS_PIN 27    // Pulse-Per-Second Pin used for GPS Time Synchronization
#define PPSOffsetSeconds 0
#define PPSOffsetMicroseconds 0
#define GPSSerial Serial1
#define GPSBaudRate 115200

// WiFi Network(s) for Setup/Initialization - Any additions or subtractions made here also need to be made to setWifiConfig() in Functions.cpp
#define WIFICONNECTTIME 30         // How many seconds to wait while attempting to connect to a network before moving on
#define I_WIFI_SSID "fgcu-campus"  // Update This! <--------------------------------------------------------------------------
#define I_WIFI_PASSWORD ""         // Update This! <---------------------------------------------------------------------------------
#define I_WIFI_SSID2 ""            // Alternate network for time sync only // Update This! <--------------------------------------------
#define I_WIFI_PASSWORD2 ""        // Update This! <--------------------------------------------------------------------------------
#define I_WIFI_ATTEMPT_COUNT_LIMIT 3
// WiFi Network(s) for Operation - Any additions or subtractions made here also need to be made to setWifiMultiConfig() in Functions.cpp
#define O_WIFI_SSID I_WIFI_SSID
#define O_WIFI_PASSWORD I_WIFI_PASSWORD
// #define O_WIFI_SSID2 ""
// #define O_WIFI_PASSWORD2 ""

// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://69.88.163.33:8086"  // Update This! <------------------------------------------------------------
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "478mGdfNVYM9gbTEn7X6kSPsy659RJo1zu_Znc0jW-VWsekCP4URT11zZSrV_0XRzhURcux58EQD9wbP_Azfpw=="  // Update This! <----------------
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "867116c343b9f084"  // Update This! <--------------------------------------------------------------------
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "OutdoorESP1"  // Update This! <-----------------------------------------------------------------------------

// MQTT Server information
#define MQTT_SERVER "69.88.163.33"
#define MQTT_PORT 1883
#define MQTT_TOPIC_SUBCRIBE "topic/fromNR"
#define MQTT_TOPIC_PUBLISH "topic/toNR"

// NODE-RED Commands
#define NODE_RED_START "start"
#define NODE_RED_STOP "stop"
#define NODE_RED_RESET "reset"

// Transmission Batching Controls
#define BATCH_SIZE 150
#define BATCH_SIZE 250
#define StartTransmissionPercentage 50
#define HighRateMutexWaitTicks 10
#define InfluxSequentialTransmitLimit 10

// Time
#define TimeZoneOffset "EST+5EDT,M3.2.0/2,M11.1.0/2"  // Check This! <--------------------------------------------------------
#define ntpServer "pool.ntp.org"
#define ntpServer2 "time.nis.gov"

// SD Card (Adalogger)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define FILENAME "/" STR(DEVICE) "_Log.txt"


// Global Variables:
/*****************************************************************************/
#ifdef HasNeopixel
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif
#ifdef InfluxLogging
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point* fast_datapoints[BATCH_SIZE];
SemaphoreHandle_t InfluxClientMutex = NULL;
#endif
#ifdef SDLogging
fs::File dataLog;
#endif
#ifdef OLEDDebugging
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
#endif
bool writeError = false;
bool GPSSync = false;
int slowPointCount = 0;
int fastPointCount = 0;
int fastPointCountAlt = 0;
// struct timeval tv;
unsigned long GPS_us;
TaskHandle_t Task1;
TaskHandle_t Task2;
TinyGPSPlus gps;
WiFiMulti wifiMulti;
EspMQTTClient mqttClient(
  MQTT_SERVER,
  MQTT_PORT,
  DEVICE);


// Configuration Sanity Checks
#if !defined(InfluxLogging) && !defined(SDLogging)
#error No Data Logging Selected
#endif

#if defined(SDRedundantLoggingOnly) && !defined(SDLogging)
#error SD Logging must be enabled to use SD Redundant Logging
#endif

#endif  // ConfigCode