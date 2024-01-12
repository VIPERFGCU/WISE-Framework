// ESP_Sensor_Framework_Template.ino  --  V0
// Framework for running a high-rate hardware timer based sensor polling scheme on core 1 and a low rate
// sensor polling scheme on core 0, with data transmission handled by core 0 and time accuracy updates 
// using GPS handled by core 1
// Written by: Jordan Kooyman

// Pin Definitions
#define GPS_PPS_Pin 13


// Global Variables:
TaskHandle_t Task1;
TaskHandle_t Task2;


// Function Prototypes:
// Core0.ino
void core0Setup();
void core0Loop();
void core0OS(void * pvParameters);

// Core1.ino
void core1Setup();
void core1Loop();
void core1OS(void * pvParameters);

// Interrupt_Service_Routines.ino
static void Sensor0_Callback(void* args);
void IRAM_ATTR GPS_PPS_ISR();


void setup()
{  //create a task that will be executed in the core0OS() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    core0OS,   /* Task function. */
                    "Core 0",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the core1OS() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    core1OS,   /* Task function. */
                    "Core 1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}

void core0OS(void * pvParameters)
{
  //core0Setup();
  while(true)
  {
    //core0Loop();
    digitalRead(0);
  }
}

void core1OS(void * pvParameters)
{
  //core1Setup();
  while(true)
  {
    //core1Loop();
    digitalRead(1);
  }
}

void loop()
{
    // Do Nothing!
}

