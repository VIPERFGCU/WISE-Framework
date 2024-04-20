# WISE-Framework
The Wireless Intelligent Sensor Ecosystem (WISE) Framework is an open-source modular project with examples for managing and deploying low-cost sensors for bulk data collection, with a focus on structural and environmental data collection.

This project was originally intended to be a single accelerometer affixed to a structure, such as a bridge, to monitor the structural health by observing the transmitted frequencies and harmonics.

This project framework currently offers the end user the ability to connect as many sensors as they want, within the physical limitations of the microcontroller, and manages the collection of the data.

## Features
* High-rate polling (up to about 600hz between all connected sensors)
* Low-rate polling (unlimited interval (in seconds) between sensor polling
* Independent polling speed for each sensor
* Modular design using low-cost readily available Arduino-compatible sensors and microcontrollers
* Self-sufficient design operated from solar power, designed to be used outdoors
* Central time-series database with InfluxDB (locally- or cloud-hosted)
* Data visualization utilizing Grafana
* Remote sensor management using NodeRed

## Example Use Cases
* Structure monitoring with accelerometers
* Rainfall monitoring
* Water/liquid level monitoring in tanks or outdoors
* Air quality monitoring
* Mini weather station(s)
* Vehicle/vessel-mounted trip/environmental monitoring

## How it Works
The framework is designed around the ESP32 S-series microcontroller family and utilizes both cores. 

Core 0 is tasked with handling interrupts to process all time-sensitive sensor polling and data storage as well as the time resync through the GPS module.

Core 1 is tasked with managing all the low-rate sensors using software timers as well as handling the wireless connection to the remote database and the initial setup procedure during boot-up.

## Hardware
The project is based around an ESP32 microcontroller, with an attached GPS module for real-time time synchronization. Connect any compatible sensor to the ESP32, and that represents the core of this project.

Optionally, you can add more sensors, a battery, a solar panel with a charge controller, an SD Card, a display, a LoRa radio, or anything else compatible with the ESP32. Put it all in a water-resistant box, and it can be deployed outside for remote sensing tasks.

A [sample BOM](Sensor%20BOM.xlsx) is provided, primarily utilizing Adafruit as a vendor. The Feather series of development boards makes things very easy to prototype with and develop small-batch projects.

## Software Setup
The code is designed such that most configurations will take place exclusively in [Configuration.h](ESP_Sensor_Framework_Template/Code/Configuration.h).

Support for additional sensor modules will need to be added to the appropriate sensor support file, depending on the frequency it needs to be polled at.

The template code is available here, designed to be modified and uploaded to the ESP32 using Arduino IDE.

## Server Setup


## ToDo & Future Expansions
- [X]  Publish project to GitHub
- [X]  Test on multiple servers
- [X]  Add SD Card Support for local data logging
- [ ]  Test the time synchronization with the GPS module
- [ ]  Test the water resistance of the hardware box
- [ ]  Add a license to this repository
- [ ]  Expand and improve documentation
- [ ]  Validate long-term durability of design, physical and digital (is heat an issue?)
- [ ]  Restructure the codebase to make each sensor's functions a separate source file that can be included or excluded with only a line or two of code (compiler directives?)
- [ ]  Test and improve high-rate data transfer to InfluxDB
- [ ]  Validate and reliability test multiple sensors to one MCU, some with long wires
- [ ]  Improve NodeRed control to allow individual sensor management
- [ ]  Add deep sleep to ESP, controlled by NodeRed
- [ ]  Add battery level monitoring
- [ ]  Implement power conservation designs, utilizing deep sleep
- [ ]  Setup more Grafana data visualization and analysis suites
- [ ]  Update transmission protocol to retry sending data that was saved to SD due to transmission failure
- [ ]  Add ability for data transfer to another device or internet when in range, but it collects data continuously
- [ ]  Add serial connectivity to allow data to be sent to a secondary microcontroller for AI-based anomaly detection
- [ ]  Add Lo-Ra support for long-distance status updates
- [ ]  Add BLE support for an ad-hoc or other network structure, with only 1 or few nodes transmitting to the internet
- [ ]  Improve security and data integrity
- [ ]  Add additional sensor modules to the base code
- [ ]  Add support for/test on other MCUs, such as ESP8266
- [ ]  Add real demos of this system in use
