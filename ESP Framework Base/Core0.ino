// Core0.ino
// Low rate sensor polling and data transmission handling framework
// Written by: Jordan Kooyman

// Global Variables:



void core0Setup()
{
  digitalRead(1);
}

void core0Loop()
{
  digitalRead(2);

  // Handle low-rate data polling and packing using a software timer here

  // Handle packaged data transmission using a software timer here, as an interrupt?
}