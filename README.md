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

## How it Works
The framework is designed around the ESP32 S-series microcontroller family and utilizes both cores. 

Core 0 is tasked with handling interrupts to process all time-sensitive sensor polling and data storage as well as the time resync through the GPS module.

Core 1 is tasked with managing all the low-rate sensors using software timers as well as handling the wireless connection to the remote database and the initial setup procedure during boot-up.

## Hardware


## Hardware Setup


## Server Setup


## Future Expansions
* Improve NodeRed control to allow individual sensor management
* Update transmission protocol to retry sending data that was saved to SD due to transmission failure
* Add serial connectivity to allow data to be sent to a secondary microcontroller for AI-based anomaly detection
* Add Lo-Ra support for long-distance status updates
