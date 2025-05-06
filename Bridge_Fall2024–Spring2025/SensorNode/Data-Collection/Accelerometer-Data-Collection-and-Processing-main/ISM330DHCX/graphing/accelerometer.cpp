#include "accelerometer.h"

Adafruit_ISM330DHCX ism330dhcx;

void accelerometer_Setup(){
  //while (!Serial){delay(10); // will pause until serial console opens}

  if (!ism330dhcx.begin_I2C()) {
    // if (!ism330dhcx.begin_SPI(LSM_CS)) {
    // if (!ism330dhcx.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) {
    Serial.println("Failed to find ISM330DHCX chip");while (1) {delay(10);}
  }

  /*
  LSM6DS_ACCEL_RANGE_2_G:
  LSM6DS_ACCEL_RANGE_4_G:
  LSM6DS_ACCEL_RANGE_8_G:
  LSM6DS_ACCEL_RANGE_16_G:
  */

  ism330dhcx.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
  /*
  LSM6DS_RATE_SHUTDOWN
  LSM6DS_RATE_12_5_HZ
  LSM6DS_RATE_26_HZ
  LSM6DS_RATE_52_HZ
  LSM6DS_RATE_104_HZ
  LSM6DS_RATE_208_HZ
  LSM6DS_RATE_416_HZ
  LSM6DS_RATE_833_HZ
  LSM6DS_RATE_1_66K_HZ
  LSM6DS_RATE_3_33K_HZ
  LSM6DS_RATE_6_66K_HZ
  */

  ism330dhcx.setAccelDataRate(LSM6DS_RATE_104_HZ);
  

  // Maybe
  ism330dhcx.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
  ism330dhcx.setGyroDataRate(LSM6DS_RATE_208_HZ);

  ism330dhcx.configInt1(false, false, true); // accelerometer DRDY on INT1
}

void accelerometer_loop(){
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  ism330dhcx.getEvent(&accel, &gyro, &temp);
  //Serial.print(millis());
  //Serial.print(",");
  Serial.print(accel.acceleration.x, 6);
  Serial.print(",");
  Serial.print(accel.acceleration.y, 6);
  Serial.print(",");
  Serial.print(accel.acceleration.z, 6);
  Serial.println();
}
