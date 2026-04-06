/* Author: Siang Chin
 * Desc: This file standardized the acceleroiter data with a timestamp.
 * Call create_timestamped_accel_data to create a timestamp of the acceleration data in nanoseconds
 * It uses ntp_client's getEpochTime() to get the time
 * All acceceration is scaled so 9.8m/s is 1.0. This is done to account for the
 * many different sensors present.( Ex: Some may report an integer, while other libraries may report a float. )
 * Each sensor has 2 functions:
 *  void get_accelerometer_data(float data[3])
 *  void init_accelerometer()
 */

#ifndef ACCEROMETER_H
#define ACCEROMETER_H
#include <stdint.h>

#define USE_ISM 1
#define USE_LSM 0

struct TimeStampedAccelData {
    uint64_t timestamp; 
    float ax; 
    float ay; 
    float az; 
};

void init_accelerometer();
void get_accelerometer_data(float data[3]);
void create_timestamped_accel_data(TimeStampedAccelData &data_struct, const float &raw_x, const float &raw_y, const float &raw_z);

#endif /* ACCEROMETER_H */