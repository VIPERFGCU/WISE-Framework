# Tflite-Data-Collection-and-Processing

Configures TensorFlow Lite, uses ISM330DHCX, gets serial data into a csv, and graphs Raw X, Denoised X, RMS, RMSE, MSE.


## What is this for?

This project is part of a bridge monitoring system where accelerometer data is used to assess the structural health 
of small to medium-sized bridges. By capturing and analyzing vibration data, this system aids in the early detection of potential 
structural issues. The scripts provided here collect denoised data from the ISM330D sensor, save it to a CSV file, and plot it for analysis.

## How to use it

1. **Compile and upload** the tflite data collection files to your microcontroller to start data collection.

2. **Run** `SerialToCsvTflite.py` to save the serial data into a CSV file.

3. **Visualize** the data using `GraphTflite.py` to generate graphs of the collected data.

## Prerequisites

- **Hardware**:
  - ESP32 compatible with Tensorflow lite
  - Adafruit ISM330DHCX + LIS3MDL FeatherWing

## Notes

- Read all instructions thoroughly before starting data collection.
- Ensure the ESP32 and accelerometer are connected correctly to maintain data accuracy.

## Disclaimer

Use this program at your own risk. The author is not responsible for any potential damage to your system.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
