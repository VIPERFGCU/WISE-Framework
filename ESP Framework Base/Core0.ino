// Core0.ino
// Low rate sensor polling and data transmission handling framework
// Written by: Jordan Kooyman

// Global Variables:



void core0Setup()
{
  // Connect to WiFi (if available)

  // Set Initial System Epoch Time from Internet - need to change this if we use an ad-hoc bluetooth network backup option
  // Alternative methods for setting the time include getting the time over bluetooth and pulling it from GPS (only need seconds since epoch)
  configTime(0, 0, ntpServer);

  // Connect to InfluxDB and Mosquito/NodeRed

  // Setup Attached Sensors

}

void core0Loop()
{
  digitalRead(2);

  // Handle low-rate data polling and packing using a software timer here

  // Handle packaged data transmission using a software timer here, as an interrupt?

  // https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#timestamp
}