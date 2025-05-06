# Bridge: Real-Time Intelligent Detection & Geospatial Evaluation

**Team:** Nicholas Chlumsky, Joseph Lennon, Giovanni Moncibaez, John Nguyen

**Sponsor:** Dr. Ali Ozdagli

![BRIDGE](Bridge_Fall2024–Spring2025/Documentation/images/BRIDGE%20Senior%20Project%20Poster%20.png)

## 📖 Project Overview

Bridge is an edge‑AI powered structural health monitoring system designed for real‑time anomaly detection and geospatial evaluation of bridges. Utilizing onboard TensorFlow Lite inference on ESP32 microcontrollers, the system computes an anomaly index from acceleration data and transmits it via LoRaWAN for cloud storage and visualization.

## 🔑 Key Features

* **On‑Device Anomaly Detection**: Autoencoder model on ESP32 processes acceleration data and outputs a Root Mean Square Error (RMSE) based anomaly index.
* **Low‑Power LoRaWAN Transmission**: Efficient wireless delivery of computed metrics to a ChirpStack network server.
* **Cloud Storage & Visualization**: Data flows into InfluxDB on AWS and is rendered in Grafana dashboards for live monitoring and historical analysis.

## 🏗️ System Architecture

1. **Data Acquisition**: Adafruit ISM330DHCX IMU collects tri‑axial acceleration at configurable sampling rates.
2. **Edge Processing**: ESP32 runs a TensorFlow Lite Micro autoencoder to detect deviations from baseline behavior.
3. **Anomaly Index Computation**: RMSE between incoming data and reconstructed baseline signals quantifies anomalies.
4. **LoRaWAN Uplink**: LoRa module sends the anomaly index to a Raspberry Pi running ChirpStack.
5. **Data Ingestion**: Raspberry Pi forwards incoming metrics to InfluxDB hosted on AWS.
6. **Visualization**: Grafana dashboards display real‑time and historical anomaly trends across multiple sensor nodes.

## 🛠️ QUICK Tech Stack

* **Microcontroller & AI:** ESP32 Feather V2 + TensorFlow Lite Micro
* **Sensor:** Adafruit High Precision 9-DoF IMU FeatherWing (ISM330DHCX + LIS3MDL)
* **Wireless:** Adafruit LoRa Radio FeatherWing - RFM95W 900 MHz + ChirpStack on Raspberry Pi
* **Database:** InfluxDB (AWS)
* **Dashboard:** Grafana

## 🚀 Getting Started

1. **Read Final Report**
* Reading final report found here, will give you step-by-step process on how to configure and start working on bridge

## 📞 Contact & Contribution

For questions, issues, or contributions, please open an issue or reach out to the team leads via email.
