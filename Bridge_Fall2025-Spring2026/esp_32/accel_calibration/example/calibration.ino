/**
 * This code is an edit of sparkfun's H3LIS331DL calibration example.
 * https://github.com/Seeed-Studio/Accelerometer_H3LIS331DL/blob/master/examples/H3LIS331DL_AdjVal/H3LIS331DL_AdjVal.ino
 * It adds on scale calculations for more accurate computation.
 * calibrated = (raw - offset) * scale
 */

//#include <Adafruit_ISM330DHCX.h>
//Adafruit_ISM330DHCX ism330dhcx;
//void get_accelerometer_data(float data[3]) {
//  sensors_event_t accel;
//  sensors_event_t gyro;
//  sensors_event_t temp;
//  ism330dhcx.getEvent(&accel, &gyro, &temp);
//
//  data[0] = accel.acceleration.x;
//  data[1] = accel.acceleration.y;
//  data[2] = accel.acceleration.z;
//  
//}

#include <Adafruit_LSM6DSOX.h>
Adafruit_LSM6DSOX sox;
void get_accelerometer_data(float data[3]) {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  sox.getEvent(&accel, &gyro, &temp);

  data[0] = accel.acceleration.x;
  data[1] = accel.acceleration.y;
  data[2] = accel.acceleration.z;
}

//
//#include <H3LIS331DL.h> // If you are using the H3LIS331DL
//#include <Wire.h>
//
//H3LIS331DL h3lis;
//void get_accelerometer_data(uint16_t data[3]) {
//  AxesRaw_t h3lis_data;
//  h3lis.getAccAxesRaw(&h3lis_data);
//  data[0] = h3lis_data.AXIS_X;
//  data[1] = h3lis_data.AXIS_Y;
//  data[2] = h3lis_data.AXIS_Z;
//}

bool tst = false;
void IRAM_ATTR isr() {
  tst = true;
};

void setup() {
  Serial.begin(115200);
//  h3lis.init();
  
//  ism330dhcx.begin_I2C();
//  ism330dhcx.setAccelDataRate(LSM6DS_RATE_208_HZ);
//  ism330dhcx.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);

  if (!sox.begin_I2C()) {
    // if (!sox.begin_SPI(LSM_CS)) {
    // if (!sox.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) {
     Serial.println("Failed to find LSM6DSOX chip");
    while (1) {
      delay(10);
    }
  }
  
  getAdjParameter();

//  attachInterrupt(27, isr, INPUT_PULLUP); 
}

float a[3];
void loop() {
  //nothing to do
//  delay(1000);
//  get_accelerometer_data(a);
//  Serial.print("Complete: ");
//  Serial.printf("%f, %f, %f\n", a[0], a[1], a[2]);
//  if(tst) {
//    Serial.println("TSTSTSTTSSTSTSTTSTS");
//    tst = false;
//  }
}

char* tag[] = {"Z+", "Z-", "Y+", "Y-", "X+", "X-"};
float dataBuf[6][3] = {0};

void getAdjParameter() {
  float data[3];
  float sum[3] = {0};

  Serial.println("Start calibration...");
  delay(3000);
  for (int orientation = 0; orientation < 6; orientation++) {
    Serial.print("Please place the sensor with ");
    Serial.print(tag[orientation]);
    Serial.println(" up");
    delay(3000);
    for (int countdown = 5; countdown > 0; countdown--) {
      Serial.print("Start in ");
      Serial.print(countdown);
      Serial.println(" s");
      delay(1000);
    }
    Serial.println("Measuring ...");
    for (int i = 0; i < 100; i++) {
      get_accelerometer_data(data);
      sum[0] += data[0];
      sum[1] += data[1];
      sum[2] += data[2];
      delay(20);
    }
    dataBuf[orientation][0] = sum[0] / 100.0;
    dataBuf[orientation][1] = sum[1] / 100.0;
    dataBuf[orientation][2] = sum[2] / 100.0;
    sum[0] = sum[1] = sum[2] = 0;
    Serial.println("Done orientation");
    delay(1000);
  }

  // Calculate offset and scale for each axis
  // X-axis: orientation 4 = +X, 5 = -X
  float rawXp = dataBuf[4][0];
  float rawXn = dataBuf[5][0];
  float offsetX = (rawXp + rawXn) / 2.0;
  float scaleX = 1.0 / ((rawXp - rawXn) / 2.0);

  // Y-axis: orientation 2 = +Y, 3 = -Y
  float rawYp = dataBuf[2][1];
  float rawYn = dataBuf[3][1];
  float offsetY = (rawYp + rawYn) / 2.0;
  float scaleY = 1.0 / ((rawYp - rawYn) / 2.0);

  // Z-axis: orientation 0 = +Z, 1 = -Z
  float rawZp = dataBuf[0][2];
  float rawZn = dataBuf[1][2];
  float offsetZ = (rawZp + rawZn) / 2.0;
  float scaleZ = 1.0 / ((rawZp - rawZn) / 2.0);

  // Output the calibration parameters
  Serial.println("Calibration results:");
  Serial.print("X offset: "); Serial.print(offsetX);
  Serial.print(" | X scale: "); Serial.println(scaleX, 6);

  Serial.print("Y offset: "); Serial.print(offsetY);
  Serial.print(" | Y scale: "); Serial.println(scaleY, 6);

  Serial.print("Z offset: "); Serial.print(offsetZ);
  Serial.print(" | Z scale: "); Serial.println(scaleZ, 6);
}
