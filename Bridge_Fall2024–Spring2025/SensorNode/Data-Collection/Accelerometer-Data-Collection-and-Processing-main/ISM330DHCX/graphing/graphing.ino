#include "accelerometer.h"

unsigned long previousMillis = 0;
const unsigned long sampleInterval = 10;  // 10 ms interval for 100 Hz sampling

void setup(void) {
  Serial.begin(115200);
  accelerometer_Setup();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check if it's time to sample
  if (currentMillis - previousMillis >= sampleInterval) {
    // Update the timestamp, but to avoid drift add the interval instead of currentMillis.
    previousMillis += sampleInterval;

    // Read and process your sensor data here
    accelerometer_loop();  // If this prints data, it'll be output at ~100Hz
  }
}
