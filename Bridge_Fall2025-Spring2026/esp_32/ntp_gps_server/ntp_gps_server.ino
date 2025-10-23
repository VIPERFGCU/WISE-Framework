#include <WiFi.h>
#include <WiFiUdp.h>
#include <TinyGPSPlus.h>

// ================== CONFIGURATION (FOR ESP32-WROOM-32) ===================
// Board: ESP32 Dev Module
// Serial Port: 1. Pins: 16 and 17
const char* ssid     = "hotspot";
const char* password = "password";
#define PPS_PIN 4                // PPS input pin
#define GPS_RX 16
#define GPS_TX 17

// ------------------ SUB SECTION: SERVER -------------------------------
const int NTP_PORT = 123;
// ======================================================================

// GPS & UDP
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // UART1 for GPS (GPIO 17/16) // SPECIFICALLY FOR THE ESP32-WROOM-32
WiFiUDP udp;
byte packetBuffer[48];
byte packetBufferFromRequest[48];

// Timekeeping
volatile time_t currentTime = 0;
volatile uint32_t ppsMicros = 0;
volatile bool ppsFlag = false;

// ================ MULTICORE ================
SemaphoreHandle_t timerMux; // So time does not change during ntp request processing
TaskHandle_t gpsTaskHandle = NULL;  // task handle for updating time
TaskHandle_t ntpTaskHandle = NULL;  // task handle for handling ntp server request
// =========================================


// Predeclarations
void IRAM_ATTR onPPS();
void gps_setup();

void setup() {
  timerMux = xSemaphoreCreateBinary();
  xSemaphoreGive(timerMux);

  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  pinMode(PPS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PPS_PIN), onPPS, RISING);

  gps_setup();

  while(currentTime == 0) {
    yield();
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  Serial.println("Wifi Connected!");

  // Setup tasks
  NTPServerSetup();
}

uint64_t getEpochTime() {
  xSemaphoreTake(timerMux, portMAX_DELAY);
  time_t baseTime = currentTime;          // seconds since epoch
  uint32_t pps = ppsMicros;    // micros() timestamp at last PPS pulse
  xSemaphoreGive(timerMux);

  uint32_t nowMicros = micros();

  // Calculate microseconds elapsed since last PPS pulse, handling micros() overflow
  uint32_t deltaMicros;
  if (nowMicros >= pps) {
    deltaMicros = nowMicros - pps;
  } else {
    deltaMicros = (0xFFFFFFFF - pps) + nowMicros;
  }

  // Combine seconds and microseconds into uint64_t microseconds since epoch
  uint64_t epochMicros = ((uint64_t)baseTime * 1000000ULL) + deltaMicros;

  return epochMicros;
}

void loop() {
  vTaskDelete(NULL); // Delete the loop task
}

//=================GPS TIME=======================
void IRAM_ATTR onPPS() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (xSemaphoreTakeFromISR(timerMux, &xHigherPriorityTaskWoken)) {
    currentTime++;                 
    ppsMicros = micros();     
    ppsFlag = true;
    xSemaphoreGiveFromISR(timerMux, &xHigherPriorityTaskWoken);
  }
}

//============================================
//=================NTP SERVER=================
uint32_t receiveMicros = 0;
void ntp_server_task(void* parameter) {
  udp.begin(NTP_PORT);

  // Handle NTP Requests
  while(true) {
    int packetSize = udp.parsePacket();
    
    if (packetSize == 48) {
      xSemaphoreTake(timerMux, portMAX_DELAY);
      receiveMicros = micros();  
      udp.read(packetBufferFromRequest, 48);    
      sendNTPResponse(udp.remoteIP(), udp.remotePort());
      xSemaphoreGive(timerMux);
    } else {
      udp.flush();
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // 1ms pause( for watchdog reset)
  }
}


void NTPServerSetup() {
  xTaskCreatePinnedToCore(
    ntp_server_task,      // Function that should be called
    "ntp_server_task",  // Name of the task (for debugging)
    8000,                  // Stack size (bytes)
    NULL,                  // Parameter to pass
    20,                    // Task priority( Max )
    &ntpTaskHandle,          // Task handle
    0                      // use core 0 to keep seperate from arduino processes( this includes the time )
  );
}

//==========================================

void sendNTPResponse(IPAddress remoteIP, unsigned int remotePort) {
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b00100100;  // LI = 0, Version = 4, Mode = 4 (server)
  packetBuffer[1] = 1;           // Stratum 1 (GPS)
  packetBuffer[2] = 6;           // Poll interval
  packetBuffer[3] = 0xEC;        // Precision (-20)

  // Reference ID ("GPS")
  packetBuffer[12] = 'G';
  packetBuffer[13] = 'P';
  packetBuffer[14] = 'S';
  packetBuffer[15] = 0;

  // Get base seconds and PPS micros at receive time
  time_t now = currentTime;
  uint32_t secondsSince1900 = now + 2208988800UL;
  
  // Reference Timestamp (usually system clock set time)
  writeTimestamp(16, secondsSince1900, 0);
  
  // Receive timestamp: captured earlier when packet received
  uint32_t receiveFraction = ((uint64_t)(receiveMicros - ppsMicros) * 4294967296ULL) / 1000000ULL;
  writeTimestamp(32, secondsSince1900, receiveFraction);
  
  // Transmit timestamp: capture current time *right before sending*
  uint32_t transmitMicros = micros();
  uint32_t transmitFraction = ((uint64_t)(transmitMicros - ppsMicros) * 4294967296ULL) / 1000000ULL;
  writeTimestamp(40, secondsSince1900, transmitFraction);

  // === Originate Timestamp === (copied from client)
  memcpy(&packetBuffer[24], &packetBufferFromRequest[40], 8);

  udp.beginPacket(remoteIP, remotePort);
  udp.write(packetBuffer, 48);
  udp.endPacket();
}


void writeTimestamp(int start, uint32_t seconds, uint32_t fraction) {
  packetBuffer[start]     = (seconds >> 24) & 0xFF;
  packetBuffer[start + 1] = (seconds >> 16) & 0xFF;
  packetBuffer[start + 2] = (seconds >> 8)  & 0xFF;
  packetBuffer[start + 3] = (seconds)       & 0xFF;

  packetBuffer[start + 4] = (fraction >> 24) & 0xFF;
  packetBuffer[start + 5] = (fraction >> 16) & 0xFF;
  packetBuffer[start + 6] = (fraction >> 8)  & 0xFF;
  packetBuffer[start + 7] = (fraction)       & 0xFF;
}

// ============================================

const int GPS_DATA_AGE_THRESHOLD = 5000; // 50 seconds( in ms )
void gps_update_time(void *parameter) {  
  bool time_updated = false;
  while(!time_updated) {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }

    if (gps.date.isValid() && gps.time.isValid() && gps.location.isValid()) {
      // Check for year validity
        struct tm t;
  
        t.tm_year = gps.date.year() - 1900;
        t.tm_mon  = gps.date.month() - 1;
        t.tm_mday = gps.date.day();
        t.tm_hour = gps.time.hour();
        t.tm_min  = gps.time.minute();
        t.tm_sec  = gps.time.second() + 1;  // We update the time after the pps flag indicates that the next second has begun
        t.tm_isdst = 0;

        xSemaphoreTake(timerMux, portMAX_DELAY);
        ppsFlag = false;
        xSemaphoreGive(timerMux);
        
        // Wait for PPS pulse
        bool ppsSeen = false;
        while (!ppsSeen) {
          xSemaphoreTake(timerMux, portMAX_DELAY);
          ppsSeen = ppsFlag;
          xSemaphoreGive(timerMux);
          delay(10);
        }

        xSemaphoreTake(timerMux, portMAX_DELAY);
        currentTime = mktime(&t);
        ppsMicros = micros(); // Set reference point
        ppsFlag = false;
        xSemaphoreGive(timerMux);
  
        time_updated = true;
    }
    delay(10);
  }

  vTaskDelete(NULL);
}

void gps_setup() {
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // Keep reading and parsing GPS data until we get a valid time fix
  xTaskCreatePinnedToCore(
    gps_update_time,      // Function that should be called
    "gps_update_time",  // Name of the task (for debugging)
    15000,                  // Stack size (bytes)
    NULL,                  // Parameter to pass
    1,                    // Task priority
    &gpsTaskHandle,          // Task handle
    1                      // use core 0 to keep with arduino processes
  );
}
