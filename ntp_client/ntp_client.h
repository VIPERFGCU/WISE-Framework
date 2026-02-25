#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>
#include "time.h"

// Headers
time_t getNtpTime(IPAddress& ntpServer);
uint64_t getEpochTime();
void ntp_setup();

// ========= NTP ===========
WiFiUDP udp;
//const int PPS_PIN = 4;  // WROOM
const int PPS_PIN = 27; // V2 - A Wroom
//const int PPS_PIN = 7 // unexpected maker v2
const int NTP_PORT = 123;
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
//IPAddress ntpServer(192,168,0,2); // The esp32 ntp server
//IPAddress ntpServer(129, 6, 15, 26); // nist ntp server. time.google.com has blocked me as well as nist servers: 129.6.15.28 and 129.6.15.29
IPAddress ntpServer(10,100,100,1);  // Local ntp server( on linux )
// =========================
// ====TIME SETTINGS=====
const long  gmtOffset_sec = 0;     // Set your timezone offset in seconds
const int   daylightOffset_sec = 0;
// ---TIME TRACKING-----
volatile time_t currentTime = 0;   
volatile uint32_t lastPpsMicros = 0;
volatile bool ppsFlag = true; // Start with pps disabled
volatile uint32_t microsecondAccumulator = 0;
// ======================

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onPPS() {
  portENTER_CRITICAL_ISR(&timerMux);
  uint32_t ppsMicros = micros();

  // Calculate the delta using unsigned math (handles micros() wrapping)
  uint32_t deltaMicros = ppsMicros - lastPpsMicros;

  microsecondAccumulator += deltaMicros;

  time_t secondsToAdd = 0;
  // Add as many full seconds as we have accumulated
  while (microsecondAccumulator >= 1000000) {
    secondsToAdd++;
    microsecondAccumulator -= 1000000; // Subtract one second
  }

  currentTime += secondsToAdd;
  
  lastPpsMicros = ppsMicros;
  ppsFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void init_time_task() {
  while (currentTime == 0) {
    unsigned long delayMicros = 0;
    time_t now = getNtpTime(ntpServer);
    
    if (now == 0) {
      Serial.println("No NTP response, retrying...");
      delay(1000);
      continue;
    }
    portENTER_CRITICAL(&timerMux);
    lastPpsMicros = micros();
    ppsFlag = false;
    portEXIT_CRITICAL(&timerMux);
    
    Serial.println("Waiting for pps pulse.");
    // Wait for PPS pulse
    bool ppsSeen = true;
    while (!ppsSeen) {
      portENTER_CRITICAL(&timerMux);
      ppsSeen = ppsFlag;
      portEXIT_CRITICAL(&timerMux);
      delay(20);
    }

    portENTER_CRITICAL(&timerMux);
    currentTime = now;  // We update the time after the pps flag indicates that the next second has begun
    uint32_t ppsMicros = micros();
    currentTime += int((ppsMicros - lastPpsMicros) / 1000);  // Floor divide by the last updated time to account for time passed since pps was updated         
    lastPpsMicros = ppsMicros;
    ppsFlag = false;
    portEXIT_CRITICAL(&timerMux);

    Serial.printf("Time set to %ld at micros %u\n", currentTime, lastPpsMicros);

    delay(1000);
  }
  
  Serial.println("init_time_task done, deleting task.");
}


void ntp_setup() {
  // Setup PPS pin interrupt
  pinMode(PPS_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PPS_PIN), onPPS, RISING);
  
  // Create init_time task
  init_time_task();
}

// ========================= NTP CLIENT =================
unsigned long sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // NTP request header settings
  packetBuffer[0] = 0x1B; // Standard client request header: LI=0, Version=3, Mode=3
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  udp.beginPacket(address, 123); // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();

  // Return the time (in microseconds) just after sending packet
  return micros();
}

time_t getNtpTime(IPAddress& ntpServer) {
  udp.begin(NTP_PORT);  // Start listening on NTP port

  Serial.println("Waiting on response");
  sendNTPpacket(ntpServer);  // Send request

  // Wait forever for a response
  while (udp.parsePacket() == 0) {
    delay(10);  // Poll every 10ms
  }
  Serial.println("Response obtained!");
  int len = udp.read(packetBuffer, NTP_PACKET_SIZE);
  if (len < NTP_PACKET_SIZE) {
    Serial.println("Malformed Response");
    udp.stop();
    return 0;  // Malformed response
  }

  Serial.println("Updating time...");
  // Extract the 32-bit transmit time (seconds since 1900)
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord  = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = (highWord << 16) | lowWord;

  // Convert to Unix epoch (seconds since 1970)
  const unsigned long seventyYears = 2208988800UL;
  time_t epoch = secsSince1900 - seventyYears;

  udp.stop();
  return epoch;
}

// ===================================
// ========== TIME RETRIEVAL =========
uint64_t getEpochTime() {
  portENTER_CRITICAL(&timerMux);
  time_t baseTime = currentTime;          // seconds since epoch
  uint32_t ppsMicros = lastPpsMicros;    // micros() timestamp at last PPS pulse
  portEXIT_CRITICAL(&timerMux);

  uint32_t nowMicros = micros();

  uint32_t deltaMicros;
  if (nowMicros >= ppsMicros) {
    deltaMicros = nowMicros - ppsMicros;
  } else {
    deltaMicros = (0xFFFFFFFF - ppsMicros) + nowMicros;
  }

  // microseconds since epoch
  return ((uint64_t)baseTime * 1000000ULL) + deltaMicros;
}
