#include <HardwareSerial.h>
#include <microtime.h>
#include <microTimeLib.h>
#include "DateTime.h"



HardwareSerial Serial1(1);
HardwareSerial Serial2(2);

// GPS init
#define RXPin 32
#define TXPin  10
#define GPSBaud 115200
#define gpsTimeOffset 2 //centisecond raw offset, compared to known-good stratum 1 server

const String GPS_CODE_ZDA = "GPZDA";
String tmp;

uint8_t count_;
uint8_t parity_;

bool isNotChecked;
bool validCode;
bool validString;
bool isUpdated_;
bool getFlag_;

bool debug_;
String msg;

class GPSDateTime {
 public:
  GPSDateTime() {};

  //void commit(void);
  
/**
 * Save new date and time to private variables
 */
void commit() {
  time_ = newTime_;
  year_ = newYear_;
  month_ = newMonth_;
  day_ = newDay_;
  ltzh_ = newLtzh_;
  ltzn_ = newLtzn_;
}
  DateTime now(void);

  void time(String time) {
    newTime_ = time.toFloat() * 100;
  }

  uint16_t hour() {
    return time_ / 1000000;
  }

  uint16_t minute() {
    return (time_ / 10000) % 100;
  }

  uint16_t second() {
    return (time_ / 100) % 100;
  }

  uint32_t centisecond() {
    return time_ % 100;
  }

  void day(String day) {
    newDay_ = day.toInt();
  }
  uint16_t day(void) { return day_; };

  void month(String month) {
    newMonth_ = month.toInt();
  }
  uint16_t month(void) { return month_; };

  void year(String year) {
    newYear_ = year.toInt();
  }
  uint16_t year(void) { return year_; };

  void ltzh(String ltzh) {
    newLtzh_ = ltzh.toInt();
  }
  uint16_t ltzh(void) { return ltzh_; };

  void ltzn(String ltzn) {
    newLtzn_ = ltzn.toInt();
  }
  uint16_t ltzn(void) { return ltzn_; };

 protected:
  uint32_t newTime_;
  uint16_t newYear_, newMonth_, newDay_;
  uint16_t newLtzh_, newLtzn_;

  uint32_t time_;
  uint16_t year_, month_, day_;
  uint16_t ltzh_, ltzn_;
};


GPSDateTime datetime_;

/**
 * Send message
 * @param msg uint8_t array
 * @param len uint8_t
 */
void msendMessage(uint8_t *msg, uint8_t len, HardwareSerial port) 
{
  int i = 0;
  for (i = 0; i < len; i++) 
  {
    port.write(msg[i]);
    // Serial.print(msg[i], HEX);
  }
  port.println();
}


/**
 * Is acknowledge right from message?
 * @param  msg uint8_t array
 * @return     bool
 */
bool getAck(uint8_t *msg, HardwareSerial port) 
{
  uint8_t b;
  uint8_t ackByteID = 0;
  uint8_t ackPacket[10];
  unsigned long startTime = millis();
  Serial.print(" * Reading ACK response: ");

  // Construct the expected ACK packet
  ackPacket[0] = 0xB5;  // header
  ackPacket[1] = 0x62;  // header
  ackPacket[2] = 0x05;  // class
  ackPacket[3] = 0x01;  // id
  ackPacket[4] = 0x02;  // length
  ackPacket[5] = 0x00;
  ackPacket[6] = msg[2];  // ACK class
  ackPacket[7] = msg[3];  // ACK id
  ackPacket[8] = 0;   // CK_A
  ackPacket[9] = 0;   // CK_B

  // Calculate the checksums
  uint8_t i = 2;
  for (i = 2; i < 8; i++)
  {
    ackPacket[8] = ackPacket[8] + ackPacket[i];
    ackPacket[9] = ackPacket[9] + ackPacket[8];
  }

  while (1) 
  {
    // Test for success
    if (ackByteID > 9) 
    {
      // All packets in order!
      Serial.println(" (SUCCESS!)");
      return true;
    }

    // Timeout if no valid response in 3 seconds
    if (millis() - startTime > 3000) 
    {
      Serial.println(" (FAILED!)");
      return false;
    }

    // Make sure data is available to read
    if (port.available()) 
    {
      b = port.read();

      // Check that bytes arrive in sequence as per expected ACK packet
      if (b == ackPacket[ackByteID]) 
      {
        ackByteID++;
        //Serial.print(b, HEX);
      } else 
      {
        ackByteID = 0;  // Reset and look again, invalid order
      }
    }
  }
}

/**
 * Config code to output code for NEO 6M
 * @param  input  uint8_t array
 * @param  length int
 * @return        uint8_t array
 */
uint8_t *s2ck(uint8_t *input, int length) 
{
  uint8_t result[length + 4];

  result[0] = 0xb5;
  result[1] = 0x62;

  int i = 0;
  uint8_t CK_A = 0;
  uint8_t CK_B = 0;

  for (i = 0; i < length; i++) {
    result[i + 2] = input[i];
    CK_A += input[i];
    CK_B += CK_A;
    CK_A &= 0xff;
    CK_B &= 0xff;
  }

  result[length + 2] = CK_A;
  result[length + 3] = CK_B;

  // for (i = 0; i < length + 4; i++) {
  //   Serial.print(result[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println();

  return result;
}

/**
 * String to string with checksum
 * @param  input String
 * @return       String
 */
String s2ckv0(String input) 
{
  int i = 0;
  uint8_t checksum = 0;

  for (i = 0; i < input.length(); i++) 
  {
    checksum ^= (uint8_t) input[i];
  }

  String CK = String(checksum, HEX);
  CK.toUpperCase();

  String result = "$" + input + "*" + CK + "\r\n";

  return result;
}



extern void GPSsetup()
{

    Serial2.begin(9600, SERIAL_8N1, 32,10);


    unsigned char cfg_prt[28] = { // Baud 115200
        0xB5,0x62,0x06,0x00,0x14,0x00,
        0x01,0x00,0x00,0x00,0xD0,0x08,0x00,0x00,
        0x00,0xC2,0x01,0x00,0x07,0x00,0x03,0x00,
        0x00,0x00,0x00,0x00,
        0xC0,0x7E };
    
    Serial.print("setting GPS baud rate to 115200...");
    msendMessage(cfg_prt, 28, Serial2);
    delay(400);
    Serial.println("ok.");
    delay(600);
    Serial.println("Restarting GPS comm UART...");
    Serial2.flush();
    delay(100);
    Serial2.end();
    delay(100);
    Serial1.begin(115200, SERIAL_8N1, 32,10);
    delay(100);
    Serial.println("Now using Serial1 @115200 for GPS");

    Serial.println("disabling all NMEA messages by setting rates to 0");

    Serial1.print(s2ckv0("PUBX,40,GBS,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GGA,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GLL,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GLQ,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GNQ,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GNS,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GPQ,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GRS,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GSA,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GST,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,GSV,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,RMC,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,TXT,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,VTG,0,0,0,0"));
    Serial1.print(s2ckv0("PUBX,40,ZTA,0,1,0,0"));



    //////////
    /// CFG //
    //////////
    /// 0xB5 0x62 <class> <id> <len> <payloads> CK_A CK_B
    /// ^----^-- CONST    ^-- ID     ^-- field depends of LENGTH
    ///           ^-- CLASS    ^-- LENGTH       ^----^-- Checksum
    ///
    /// Checksum:
    ///  buffer[N];
    ///  CK_A = 0, CK_B = 0
    ///  for (int i = 0; i < N; i++) {
    ///    CK_A = CK_A + buffer[i];
    ///    CK_B = CK_B + CK_A;
    ///  }
    ///  make sure to mask both CK_A and CK_B with 0xFF
    ///  after both operations in the loop
    //////////

    ///////////////
    /// CFG-RATE //
    ///////////////
    /// Set rate for clock
    /// Packet:
    ///  CLASS: 0x06
    ///  ID: 0x08
    ///  LENGTH: 0x06
    ///  Payloads:
    ///    measRate (U2, ms): 60 (MIN)
    ///    navRate (U2, cycles): 1 (CONST)
    ///    timeRef (U2, -): 1 (GPS), 0 (UTC)
    ///
    /// Example:
    ///   B5 62 06 08 | 06 00 | 3C 00 | 01 00 | 01 00 | 52 22
    ///////////////

    // B5 62 06 01 08 00 F0 08 00 00 00 00 00 00 07 5B
    
    bool flag = false;
    /*
    uint8_t cfg_rate[] = {0xb5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12};
    Serial.print("setting GPS internal clock rate to 10Hz...");
    //while (!flag) {
        // s2ck(cfg_rate, 10)
        msendMessage(cfg_rate, 14, Serial1);
        //flag = getAck(cfg_rate, Serial1);
    //}
    delay(1000);
    Serial.println("ok.");
    flag = false;
    */

    /*
    uint8_t cfg_timepulse[] = { // off when not in sync; 1000 PPS when GPS clock in sync
    0xB5, 0x62, 0x06, 0x31, 0x20, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x40, 0x42, 
    0x0F, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0xF7, 0x00, 0x00, 0x00, 0x61, 0x7F
    };
*/

    uint8_t cfg_timepulse[] = { // off when not in sync; 1 PPS when GPS clock in sync
    0xB5, 0x62, 0x06, 0x31, 0x20, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x40, 0x42, 
    0x0F, 0x00, 0x40, 0x42, 0x0F, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x20, 0xA1, 0x07, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0xB7, 0x00, 0x00, 0x00, 0x2B, 0x1B
    };


    Serial.print("setting GPS synced timepulse output to exactly 1 PPS with 100ms puls length...");
    //while (!flag) {
        // s2ck(cfg_rate, 10)
        msendMessage(cfg_timepulse, 40, Serial1);
        //flag = getAck(cfg_rate, Serial1);
    //}
    delay(1000);
    Serial.println("ok.");
    flag = false;



    ///////////////
    /// CFG-CNSS //
    ///////////////
    /// Config GLONASS
    /// Packet:
    ///  CLASS: 0x06
    ///  ID: 0x3E
    ///  LENGTH: 4 + 8 * numConfigBlocks
    ///  Payloads:
    ///   msgVer (U1, -): 0
    ///   numTrkChHw (U1, -): READONLY
    ///   numTrkChUse (U1, -): <== numTrkChHw
    ///   numConfigBlocks (U1, -):
    ///   repeat block (numConfigBlocks times):
    ///     gnssId (U1, -)
    ///     resTrkCh (U1, -): minimum tracking channels
    ///     maxTrkCh (U1, -): >= resTrkCh
    ///     reserved1 (U1, -)
    ///     flags (X4, -): 0 in last right position
    ///
    /// Example (for GLONASS {gnssId: 0x06}):
    ///   B5 62 06 3E 24 00 00 00 16 04 00 04 FF 00 00 00 00 01 01 01 03 00 00 00 00 01 05 00 03 00 00 00 00 01 06 08 FF 00 01 00 00 01 A4 0D
    ///////////////

    uint8_t cfg_gnss[] = {0xB5, 0x62, 0x06, 0x3E, 0x0C,
        0x00, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x04, 0xFF, 0x00, 0x01, 0x00, 0x00, 0x01,
        0x56, 0xCE};
    Serial.print("setting GNSS config...");
    //  while (!flag) {
        msendMessage(cfg_gnss, 20, Serial1);
    //  flag = getAck(cfg_gnss);
    //}
    delay(800);
    Serial.println("done.");
    flag = false;
    /*
    // Save config
    // B5 62 06 09 0D 00 00 00 00 00 FF FF 00 00 00 00 00 00 17 31 BF
    uint8_t cfg_cfg[] = {0xB5, 0x62, 0x06, 0x09, 0x0D,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x17, 0x31, 0xBF};
    Serial.print("saving config...");
    while (!flag) {
        sendMessage(cfg_cfg, 21);
        flag = getAck(cfg_cfg);
    }
    Serial.println("config saved.");

    flag = false;
    */
    Serial.print("Turning off NMEA message type \"GxZDA\"...");
    Serial1.print(s2ckv0("PUBX,40,ZDA,0,0,0,0"));
    Serial.println("done.");
    
}



/**
 * Encode line to instance
 * $GPZDA,174304.36,24,11,2015,00,00*66
 * $0    ,1        ,2 ,3 ,4   ,5 ,6 *7  <-- pos
 * @return   encoded
 */
bool encode() {

  char c = Serial1.read();
  //Serial.print(c);

  if (c == '$') {
    
    tmp = "\0";
    msg = "$";
    count_ = 0;
    parity_ = 0;
    validCode = true;
    isNotChecked = true;
    isUpdated_ = false;
    return false;
  }

  if (!validCode) {
    //Serial.println("no validcode");
    return false;
  }
  msg += c;
  if (c == ',' || c == '*') {
    // determinator between values
    if (debug_) {
      Serial.print("TMP: ");
      Serial.print(count_);
      Serial.print(" <> ");
      Serial.print(tmp);
      Serial.println();
    }
    switch (count_) {
      case 0: // ID
        if (tmp.equals(GPS_CODE_ZDA)) {
          validCode = true;
        } else {
          validCode = false;
        }
        break;
      case 1: // time
        datetime_.time(tmp);
        break;
      case 2: // day
        datetime_.day(tmp);
        break;
      case 3: // month
        datetime_.month(tmp);
        break;
      case 4: // year
        datetime_.year(tmp);
        break;
      case 5:
        datetime_.ltzh(tmp);
        break;
      case 6:
        datetime_.ltzn(tmp);
        break;
      default:
        break;
    }
    if (c == ',') {
      parity_ ^= (uint8_t) c;
    }
    if (c == '*') {
      isNotChecked = false;
    }
    tmp = "\0";
    count_++;
  } else if (c == '\r') {
    // carriage return, so check
    String checksum = tmp;
    String checkParity = String(parity_, HEX);
    checkParity.toUpperCase();

    validString = checkParity.equals(checksum);

    if (validString) {
      if (debug_) {
        Serial.println("Checksum is VALID!");
      }
      datetime_.commit();
      isUpdated_ = true;
      // commit datetime
    } else {
      if (debug_) {
        Serial.print("INVALID: ");
        Serial.println(checkParity);
      }
    }
  } else if (c == '\n') {
    // end of string
    tmp = "\0";
    count_ = 0;
    parity_ = 0;
    if (debug_) {
      Serial.print("MSG: ");
      Serial.println(msg);
    }
    return true;
  } else {
    // ordinary char
    tmp += c;
    if (isNotChecked) {
      // XOR of all characters from $ to *
      parity_ ^= (uint8_t) c;
    }
  }

  return false;
}



/**
 * Return instance of DateTime class
 * @return DateTime
 */
DateTime GPSnow() {
  
  /*Serial.print("GPSnow was called: ");
  String timestring = "";
  timestring += datetime_.year();
  timestring += ":";
  timestring += datetime_.hour();
  timestring += ":";
  timestring += datetime_.minute();
  timestring += ":";
  timestring += datetime_.second();
  Serial.println(timestring);
  */

  return DateTime(datetime_.year(), datetime_.month(), datetime_.day(),
    datetime_.hour(), datetime_.minute(), datetime_.second(), datetime_.centisecond());
}


/**
 * get datetime via ZDA
 * @return DateTime
 */
DateTime getZDA() 
{
  DateTime dt = 0;
  
  uint8_t length = 0;

  digitalWrite(33, HIGH);
  //debug
  Serial.println("sending \"$EIGPQ,ZDA*39\" to Serial1");
  Serial1.print("$EIGPQ,ZDA*39\r\n");
  Serial.print("Waiting for response...");
  
  while (!Serial1.available());
  while ((length < 38) ) {
    while (Serial1.available() > 0 ) 
    {
      if (encode()) 
      {
        dt = GPSnow();
        digitalWrite(33, LOW);
        
        
      }
      length++;
    }
  }
  Serial.println("ok.");
  
  
  return dt;
}
