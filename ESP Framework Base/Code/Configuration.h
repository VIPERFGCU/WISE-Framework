// Configuration.h
// Parametric Global Configuration constants used throughout entire program
// Written by: Jordan Kooyman

#ifndef ConfigCode
#define ConfigCode

// Parametric Configurations
/*****************************************************************************/
#define SerialDebugMode

// Device
#define DEVICE "ESP32"
#define GPS_PPS_PIN 13 // Pulse-Per-Second Pin used for GPS Time Synchronization
#define PPSOffsetMicroseconds 0
//#define GPSSerial Serial1
//#define GPSBaudRate 115200

// WiFi
#define WIFI_SSID "fgcu-campus"
#define WIFI_PASSWORD ""

// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://69.88.163.33:8086"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "478mGdfNVYM9gbTEn7X6kSPsy659RJo1zu_Znc0jW-VWsekCP4URT11zZSrV_0XRzhURcux58EQD9wbP_Azfpw=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "867116c343b9f084"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "test"

// Transmission Batching Controls
#define BATCH_SIZE 100
#define StartTransmissionPercentage 80
#define FastPointCollectionLockoutDistance 20 // When within this number of points, lock fast data collection during the transmission process

// Time
#define TZ_OFFSET -17000
#define ntpServer "pool.ntp.org"


// Global Variables:
/*****************************************************************************/
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point **fast_datapoints = new Point *[BATCH_SIZE];
bool HighRateSensorLockout = false;
bool writeError = false;
int slowPointCount = 0;
int fastPointCount = 0;
int fastPointCountAlt = 0;
struct timeval tv;
TaskHandle_t Task1;
TaskHandle_t Task2;
//TinyGPSPlus GPS;
WiFiMulti wifiMulti;

#endif // ConfigCode