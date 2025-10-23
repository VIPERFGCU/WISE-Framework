#include <Arduino.h>
#include <Wire.h>
#include <ETH.h>
#include <WifiUDP.h>
#include <SSD1306.h>
#include <microTime.h>
#include "microTimeLib.h"
#include "DateTime.h"



//functions from gps.cpp (too lazy to write header)
extern DateTime getZDA();
extern void GPSsetup();

//other external functions
extern void SyncToPPS();
extern HardwareSerial Serial1;

// Input for PPS clock pulse on pin 36
#define PULSE_PIN 36
volatile uint8_t numberOfInterrupts = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// global NTP timestamp storage variables
DateTime referenceTime;
DateTime receiveTime;
DateTime transmitTime;
byte origTimeTs[9];

// offset used to adjust PPS clock pulse to match known good time sources
#define TIME_OFFSET_USEC 503900 

//interval in seconds to sync system time with gps
#define TIMESYNC_INTERVAL 15

//i2c bus pins
#define SDApin 16
#define SCLpin 17

//initialize i2C OLED on esp32 I2C0 (pins 16 and 17)
SSD1306 display(0x3C, SDApin, SCLpin); // Addr, SDA, SCL
#define lcdOffsetUpper  8 // width in number of pixels for upper free space on lcd
#define lcdOffsetLeft  4 // width in number of pixels for left free space on lcd

// rocking switch connecting pin 5 with high/low; toggles between serialConfig/NTPserver mode
#define SWITCH_PIN 5
static bool SerialMirror = 0;

// built-in LED
#define LED_PIN 33

// NTP port and packet buffer
#define NTP_PORT 123
#define NTP_PACKET_SIZE 48
byte packetBuffer[NTP_PACKET_SIZE];

//UDP server (ETH.h uses WifiUDP class)
WiFiUDP Udp;
String ip = ""; // we use DHCP


void writeLCD(uint8_t row, String txt)
 {
  display.setColor(BLACK);
  display.fillRect(lcdOffsetLeft, lcdOffsetUpper+(row*14), 128, 16);
  display.setColor(WHITE);
  display.drawString(lcdOffsetLeft, lcdOffsetUpper+(row*14), txt);
  display.display();
 }


// ethernet PHY event handler
static bool eth_connected = false;
void EthEvent(WiFiEvent_t event)
{
  
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      writeLCD(1,"ETH started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      writeLCD(1,"ETH connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      digitalWrite(LED_PIN, LOW);
      ip = ETH.localIP().toString();
      writeLCD(2,"IPv4: "+ip);
      
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ip);
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      writeLCD(1,"ETH disconnected");
      writeLCD(2,"");
      digitalWrite(LED_PIN, HIGH);
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      writeLCD(2,"ETH stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}


//ISR for PPS interrupt
void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
    numberOfInterrupts++;
  portEXIT_CRITICAL_ISR(&mux);
}  


/*
// fake time provider
time_t faketimeprovider(){
  
    tmElements_t timeinfo;
    timeinfo.Hour = 14;
    timeinfo.Minute = 33;
    timeinfo.Second = 4;
    timeinfo.Month = 3;
    timeinfo.Year = 18;
    time_t time = makeTime(timeinfo);
    //Serial.println(time);
    return time;
}*/

//gps time provider
time_t gpstimeprovider()
{    
    DateTime gpstime = getZDA();
    time_t _time;
    _time = gpstime.ntptime();
    //Serial.println(gpstime.toString());
    return (_time);
}

/*
//returns microseconds since boot
long microsfraction()
{
        unsigned long tempcycle = ESP.getCycleCount();
        return ((tempcycle-cycle1)/240.0); // esp32 runs at 240MHz - adjust for other CPUs

}
*/


void printSysTime()
{
    long diff2 = microsecond();

    Serial.print("sysTime (UTC): ");
    if (hour() < 10)
        Serial.print("0");
    Serial.print(hour());
    Serial.print(":");
    if (minute() < 10)
        Serial.print("0");
    Serial.print(minute());
    Serial.print(":");
    if (second() < 10)
        Serial.print("0");
    Serial.print(second());

    // 1*10E-6... this is 1 microsecond precision
    Serial.print(",");
    if (diff2 < 10)
        Serial.print("00000");
    else if (diff2 < 100)
        Serial.print("0000");
    else if (diff2 < 1000)
        Serial.print("000");
    else if (diff2 < 10000)
        Serial.print("00");
    else if (diff2 < 100000)
        Serial.print("0");
    
    
    Serial.print(diff2);
    Serial.println();
}

/*
void printbyteConversionTest()
{
    long rnd = random(0,1000000);
        Serial.print("waiting for ");
        Serial.print(rnd);
        Serial.println(" microseconds");
        delayMicroseconds(rnd);
        long longInt = microsfraction();
        unsigned char byteArray[4];
        longInt = longInt / 10; 
        Serial.println(longInt);
        
        // convert unsigned long int to 4-byte array
        byteArray[0] = (int)((longInt >> 24) & 0xFF) ;
        byteArray[1] = (int)((longInt >> 16) & 0xFF) ;
        byteArray[2] = (int)((longInt >> 8) & 0XFF);
        byteArray[3] = (int)((longInt & 0XFF));

        Serial.write(byteArray, 4);
        Serial.println();

        unsigned long int anotherLongInt;

        // revert from byte array to unsigned long int
        anotherLongInt = ( (byteArray[0] << 24) 
                        + (byteArray[1] << 16) 
                        + (byteArray[2] << 8) 
                        + (byteArray[3] ) );

        Serial.println(anotherLongInt);

        
}
*/

void updateLCDtime()
{
  long lcdnowfraction = microsecond();
  DateTime lcdnow(now());
  
  String stime = "";
  stime += lcdnow.toString();
  stime += ",";

  if (lcdnowfraction < 10)
    stime += "00000";
  else if (lcdnowfraction < 100)
    stime += "0000";
  else if (lcdnowfraction < 1000)
    stime += "000";
  else if (lcdnowfraction < 10000)
    stime += "00";
  else if (lcdnowfraction < 100000)
    stime += "0";
  
  stime += lcdnowfraction;

  
  writeLCD(3,stime);
}


void updateDateTime(DateTime &dt)
{
    uint32_t _microsfraction;
    time_t _now = now(_microsfraction);
    _microsfraction += TIME_OFFSET_USEC;
    while (_microsfraction >= 1000000)
    {
        _microsfraction -= 1000000;
        _now++;
    }
    
    DateTime _dt(_now, _microsfraction);
    dt = _dt;
}



void setup() 
{
    // no radio needed
    WiFi.mode(WIFI_OFF); // turn off wifi radio
    btStop(); // turn off ble radio
        

    // setup PPS interrupt
    pinMode(PULSE_PIN, INPUT_PULLDOWN);
    digitalWrite(PULSE_PIN, LOW);
    attachInterrupt(digitalPinToInterrupt(PULSE_PIN), handleInterrupt, RISING);

    // setup workmode selection switch on pin 5
    pinMode(SWITCH_PIN, INPUT);
    digitalWrite(SWITCH_PIN, LOW);

    //turn builtin LED off
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    //start i2c
    Wire.begin(SDApin,SCLpin);

    // initialise the OLED
    display.init(); 
    display.clear();
    display.flipScreenVertically(); 
    //display.setFont(ArialMT_Plain_10); // does what it says
    // Set the origin of text to top left
    //display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    
    
    Serial.begin(115200);
    
    Serial.println("\r\n\r\n\r\nNTP Server started.");
    writeLCD(0,"ESP32 GPS NTP Server");
    
   
    //set fake time for debugging purposes
    //const unsigned long DEFAULT_TIME = 1357041600UL;
    //setTime(DEFAULT_TIME);
    
    GPSsetup();
    delay(1230);
    DateTime _GPStime;


    while (_GPStime.ntptime() == 0)
    {
        Serial.println("getting time now...");
        // try to set system clock to gps time initially
        // and reset microsecond offset
        _GPStime = getZDA();
    }

    
    while(timeStatus() == timeNotSet) 
     {
        Serial.println("setting system time...");
        
        setTime(_GPStime.ntptime()); 
        delay(500);
        
     }
   

    if (timeStatus() == timeSet) 
     {
        Serial.println("initial time sync successful.");

        // we sync coarse system time from gps serial time messages:
        Serial.println("setting GPS as SyncProvider... ");     
        setSyncProvider(&gpstimeprovider);

        // get time from GPS every TIMESYNC_INTERVAL seconds
        setSyncInterval(TIMESYNC_INTERVAL);
        Serial.println("ok.");
     }  

    //delay(120);
    
    Serial.println("getting NTP reference time:\r\n");
    updateDateTime(referenceTime);
    referenceTime.print();

    WiFi.onEvent(EthEvent); //attach ETH PHY event handler
    ETH.begin(); //start ETH
    Udp.begin(NTP_PORT); // start udp server
    
}


uint64_t DateTimeToNtp64(DateTime dt) 
{
    uint64_t ntpts;

    ntpts = (((uint64_t)dt.ntptime()) << 32);
    ntpts |= (uint64_t)(dt.microsfraction() * 4294.967296);

    return (ntpts);
}

/*
uint32_t DateTimeToNtpSeconds(DateTime dt) 
{
    uint32_t ntpts;
    ntpts = dt.ntptime();
    //Serial.print((unsigned long)ntpts, HEX);
    //Serial.print(":");
    return (ntpts);
}
*/

/*
uint32_t DateTimeToNtpFraction(DateTime dt) 
{

    uint32_t ntpts;
    ntpts = (double)((dt.microsfraction()) * 4294.967296);
    //Serial.println((unsigned long)ntpts, HEX);
    return (ntpts);
}
*/


// send NTP reply to the given address
void sendNTPpacket(IPAddress remoteIP, int remotePort) 
{
  // set all bytes in the packet buffer to 0
  //memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL in readme.md for details on packet fields)
  
  // LI: 0, Version: 4, Mode: 4 (server)
  //packetBuffer[0] = 0b00100100;
  // LI: 0, Version: 3, Mode: 4 (server)
  packetBuffer[0] = 0b00011100;

  // Stratum, or type of clock
  packetBuffer[1] = 0b00000001;
  
  // Polling Interval
  packetBuffer[2] = 4;

  // Peer Clock Precision
  // log2(sec)
  // 0xF6 <--> -10 <--> 0.0009765625 s
  // 0xF7 <--> -9 <--> 0.001953125 s
  // 0xF8 <--> -8 <--> 0.00390625 s
  // 0xF9 <--> -7 <--> 0.0078125 s
  // 0xFA <--> -6 <--> 0.0156250 s
  // 0xFB <--> -5 <--> 0.0312500 s 
  packetBuffer[3] = 0xF7;
  
  // 8 bytes for Root Delay & Root Dispersion
  // root delay
  packetBuffer[4] = 0; 
  packetBuffer[5] = 0;
  packetBuffer[6] = 0; 
  packetBuffer[7] = 0;
  
  // root dispersion
  packetBuffer[8] = 0;
  packetBuffer[9] = 0;
  packetBuffer[10] = 0;
  packetBuffer[11] = 0x50;
  
  
  //time source (namestring)
  packetBuffer[12] = 71; // G
  packetBuffer[13] = 80; // P
  packetBuffer[14] = 83; // S
  packetBuffer[15] = 0;

  
  // Reference Time
  uint64_t refT = DateTimeToNtp64(referenceTime);
  
  packetBuffer[16] = (int)((refT >> 56) & 0xFF);
  packetBuffer[17] = (int)((refT >> 48) & 0xFF);
  packetBuffer[18] = (int)((refT >> 40) & 0xFF);
  packetBuffer[19] = (int)((refT >> 32) & 0xFF);
  packetBuffer[20] = (int)((refT >> 24) & 0xFF);
  packetBuffer[21] = (int)((refT >> 16) & 0xFF);
  packetBuffer[22] = (int)((refT >> 8) & 0xFF);
  packetBuffer[23] = (int)(refT & 0xFF);
 
  // Origin Time
  //copy old transmit time to origtime 

  for (int i = 24; i < 32; i++)
  {
        packetBuffer[i] = origTimeTs[i-24];
        //Serial.write(origTimeTs[i-24]);
  }

  // write Receive Time to bytes 32-39
  refT = DateTimeToNtp64(receiveTime);
  
  packetBuffer[32] = (int)((refT >> 56) & 0xFF);
  packetBuffer[33] = (int)((refT >> 48) & 0xFF);
  packetBuffer[34] = (int)((refT >> 40) & 0xFF);
  packetBuffer[35] = (int)((refT >> 32) & 0xFF);
  packetBuffer[36] = (int)((refT >> 24) & 0xFF);
  packetBuffer[37] = (int)((refT >> 16) & 0xFF);
  packetBuffer[38] = (int)((refT >> 8) & 0xFF);
  packetBuffer[39] = (int)(refT & 0xFF);
  
  
  // get current time + write  as Transmit Time to bytes 40-47
  updateDateTime(transmitTime);
  refT = DateTimeToNtp64(transmitTime);
  
  packetBuffer[40] = (int)((refT >> 56) & 0xFF);
  packetBuffer[41] = (int)((refT >> 48) & 0xFF);
  packetBuffer[42] = (int)((refT >> 40) & 0xFF);
  packetBuffer[43] = (int)((refT >> 32) & 0xFF);
  packetBuffer[44] = (int)((refT >> 24) & 0xFF);
  packetBuffer[45] = (int)((refT >> 16) & 0xFF);
  packetBuffer[46] = (int)((refT >> 8) & 0xFF);
  packetBuffer[47] = (int)(refT & 0xFF);
  
  // send reply:
  Udp.beginPacket(remoteIP, remotePort);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();

}



void loop() 
{
    // interrupt mux handler; this gets called on each
    // PPS pulse / precisely at beginning of every second
    if (numberOfInterrupts > 0)
    {
        portENTER_CRITICAL(&mux);
            numberOfInterrupts=0;
            SyncToPPS();
            
        portEXIT_CRITICAL(&mux);
        
        updateDateTime(referenceTime);
        //printSysTime();
        updateLCDtime();
           
    }


    SerialMirror = digitalRead(SWITCH_PIN);

    if (SerialMirror)
    {
        writeLCD(0, "ESP32 GPS NTP Server");
        writeLCD(1, "Serial Monitor mode");
        writeLCD(2, "");

        if (Serial1.available())
        {
            Serial.write(Serial1.read());
        }

        if (Serial.available())
        {
            Serial1.write(Serial.read());
        }

    }

    else
    {
        writeLCD(0, "ESP32 GPS NTP Server");
        writeLCD(1, "NTP server mode");
        writeLCD(2, "IPv4: "+ip);

        // process NTP requests
        IPAddress remoteIP;
        int remotePort;

        int packetSize = Udp.parsePacket();

        if (packetSize) // we've got a packet 
        {
            updateDateTime(receiveTime);

            
            //store sender ip and port for later use
            remoteIP = Udp.remoteIP();
            remotePort = Udp.remotePort();

           /* 
            Serial.print("Received UDP packet with ");
            Serial.print(packetSize);
            Serial.print(" bytes size - ");
            Serial.print("SourceIP ");
            
            for (uint8_t i =0; i < 4; i++)
            {
                Serial.print(remoteIP[i], DEC);
                if (i < 3)
                {
                    Serial.print(".");
                }
            }
            
            Serial.print(", Port ");
            Serial.println(remotePort);
            */
            
            Serial.print("query: ");
            Serial.print(receiveTime.toString());
            Serial.print(",");
            Serial.print(receiveTime.microsfraction());
            Serial.println();

            digitalWrite(LED_PIN, HIGH);

            
            // We've received a packet, read the data from it
            // read the packet into the buffer
            Udp.read(packetBuffer, NTP_PACKET_SIZE);

            //get client transmit time (becomes originTime in reply packet)
            for (int i = 0; i < 8; i++)
            {
                origTimeTs[i] = packetBuffer[40+i];
            }
            
            //send NTP reply
            sendNTPpacket(remoteIP, remotePort);
            Serial.print("reply: ");
            Serial.print(transmitTime.toString());
            Serial.print(",");
            Serial.println(transmitTime.microsfraction());
            digitalWrite(LED_PIN, LOW);
            
            //output "done"
            //updateLCDtime();
            Serial.println("NTP reply sent.\r\n*********************************");
            
        }
    }
    
    // put your main code here, to run repeatedly:


}