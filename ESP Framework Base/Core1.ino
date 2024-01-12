// Core1.ino
// High rate sensor polling framework and timing accuracy verification using GPS
// Utilizing ESP Timer library to allow for 
// Written by: Jordan Kooyman

// Global Variables:
bool timer0Flag = false;
struct timeval tv;


// Sensor Timer Configurations
  // Sum of all sensor runs per second must be less than 20,000
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#_CPPv423esp_timer_create_args_t
// Sensor0 - ???
esp_timer_handle_t Sensor0Timer;
#define Sensor0RunsPerSecond 100  
const esp_timer_create_args_t Sensor0Config = {.callback = &Sensor0_Callback, .name = "Sensor 0", .skip_unhandled_events = true};


void core1Setup()
{
  // Setup GPS Module
  attachInterrupt(GPS_PPS_Pin, GPS_PPS_ISR, RISING);

  //GPSSerial.begin(GPSBaudRate);
  
  // ESP Timer Configurations
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html
  esp_timer_create(&Sensor0Config, &Sensor0Timer);

  esp_timer_start_periodic(Sensor0Timer, 1000000 / Sensor0RunsPerSecond);
}

void core1Loop()
{
  // Update Stored System Time
  gettimeofday(&tv, nullptr);

  // Non-time-critical Sensor Polling handling
  if (timer0Flag)
  {
    timer0Flag = false;
    
    // Do the timer0 event polling
    // Add to ISR any time-critical polling and store data
    // Process then package data further here

  }
}