#include <WiFi.h>
#include <WiFiUdp.h>
#include "ntp_client.h"
#include "Accerometer.h"
#include "influx_db_handler.h"

// ================== WIFI CONFIGURATION ===================
//const char* ssid     = "OnePlus 9 5G-beaa";
//const char* password = "a28x9ga4";
const char* ssid = "pop-os";
const char* password = "adminnnn";
// ======================================================================

void setup(){
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  ntp_setup();
  
  init_accelerometer();
  
  
  Serial.println("After NTP Setup");
}

const int MS_INTERVAL = 20; // 20ms per record
unsigned long previousMillis = 0;

float raw_data[3];
void loop() {
  unsigned long currentMillis = millis();

  // Check if 20 ms have passed
  if (currentMillis - previousMillis >= MS_INTERVAL) {
    previousMillis += MS_INTERVAL;  // Use += to keep timing consistent

    // --- User Code ---
    get_accelerometer_data(raw_data);
    TimeStampedAccelData A;
    create_timestamped_accel_data(A, raw_data[0], raw_data[1], raw_data[2]);
//    Serial.printf("ax: %.6f, ay: %.6f, az: %.6f\n", A.ax,A.ay, A.az);
    transmit_accelerometer_data(A);
    //--------------
  }
}
