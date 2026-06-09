
# FastAPI Backend (Docker, InfluxDB, Mosquitto)

A minimal backend interface for sensor ingest and querying, built with FastAPI. Ships with Docker Compose that brings up:
- **api**: FastAPI + Uvicorn
- **influxdb**: time-series database (v2)
- **mosquitto**: MQTT broker

> Grafana is intentionally omitted. We can add it later.

## Quick start

```bash
# 1) Copy .env.example to .env and fill in InfluxDB token after first boot
cp .env.example .env

# 2) Start the stack
docker compose up --build

# 3) Initialize InfluxDB at http://wise-net.io:8086 (set org, bucket, and generate a token)
#    Update the .env file with the token and (optionally) restart the stack.
#    You can also use docker exec to run influx setup non-interactively.

# 4) Test health
curl http://wise-net.io:8000/health

# 5) Send a sample ingest
curl -X POST http://wise-net.io:8000/api/v1/ingest   -H "Content-Type: application/json"   -d '{"device_id":"esp32-1","ts":"2025-09-25T12:00:00Z","x":1.23,"y":-0.4,"z":0.77}'

# 6) Query recent data (last 1h)
curl "http://wise-net.io:8000/api/v1/data?device_id=esp32-1&range=1h"
```

### Dev tips
- The `api` container watches files by default (uvicorn `--reload`) when using a bind mount. Edit code locally and it hot-reloads.
- Mosquitto is included. An MQTT consumer can be enabled later (see TODO in `main.py`).

## Project layout
```
fastapi-backend/
  ├─ api/
  │  ├─ Dockerfile
  │  ├─ requirements.txt
  │  └─ app/
  │     ├─ main.py
  │     ├─ deps.py
  │     ├─ schemas.py
  │     └─ routers/
  │        └─ data.py
  ├─ mosquitto/
  │  └─ mosquitto.conf
  ├─ .env.example
  ├─ docker-compose.yml
  └─ README.md
```

## Documentation

Use the in-repo docs as the source of truth:

- [Documentation Hub](docs/README.md)
- [Section README template](docs/templates/section-readme-template.md)
- [Runbook template](docs/templates/runbook-template.md)


# WISE Edge Gateway & Sensor Documentation

This section outlines the complete configuration of the Raspberry Pi edge gateway, the Sixfab LTE module integration, data pipeline verification, sensor flashing procedures, and the current status of the Over-The-Air (OTA) firmware update logic.

## 1. Raspberry Pi Initial Setup & Credentials

Use the following credentials to access the fresh Pi configuration:

- **Hostname:** `wisenet-gateway`
- **User:** `wisenet`
- **Password:** `eagle1997*`
- **SSH Access:** `ssh wisenet@<IP_ADDRESS>`

## 2. Cellular Connectivity (Sixfab LTE HAT)

The edge gateway utilizes a Sixfab LTE HAT for remote cloud connectivity.

### Driver Installation

Download and run the Sixfab QMI installer:

```bash
sudo apt-get update
wget https://raw.githubusercontent.com/sixfab/Sixfab_QMI_Installer/master/qmi_install.sh
sudo chmod +x qmi_install.sh
sudo ./qmi_install.sh
```

### Quectel Connection Manager

To enable the header and start the connection manager:

```bash
cd /opt/qmi_files/quectel-CM
sudo ./quectel-CM -s wholesale
```

### Background Service Configuration

To ensure the cellular connection persists across reboots, a systemd service was created:

```bash
sudo nano /etc/systemd/system/quectel.service
```

Paste the following:

```ini
[Unit]
Description=Quectel Cellular Connection Manager
After=network.target

[Service]
ExecStart=/opt/qmi_files/quectel-CM/quectel-CM -s wholesale
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start the service:

```bash
sudo systemctl enable quectel.service
sudo systemctl start quectel.service
```

## 3. MQTT Broker & Mesh Hotspot Config

The Pi acts as the local broker for the ESP32 mesh network and utilizes its onboard Wi-Fi chip (`wlan0`) to broadcast the mesh access point.

### Mosquitto Broker Setup

Install the broker and client tools:

```bash
sudo apt install mosquitto mosquitto-clients -y
```

Open the Mosquitto config file to allow local mesh traffic:

```bash
sudo nano /etc/mosquitto/conf.d/local.conf
```

Paste:

```text
listener 1883 0.0.0.0
allow_anonymous true
```

Restart the broker:

```bash
sudo systemctl restart mosquitto
```

### Mesh Hotspot Setup

Create the local Wi-Fi network using NetworkManager for the root node to connect to:

```bash
sudo nmcli device wifi hotspot ifname wlan0 ssid wise-gateway password "eaglemesh*"
```

> **Note:** If the hotspot does not automatically enable on boot, SSH into the Pi using its local network IP (e.g., `ssh wisenet@10.0.0.194`), run `sudo nmcli connection up Hotspot`, then connect your laptop to the hotspot and SSH using the new IP (`ssh wisenet@10.42.0.1`).

## 4. Gateway Bridge Service

The `gateway_bridge.py` script routes data from the local mesh to the cloud and is configured to run automatically on boot.

### Create the Service File

```bash
sudo nano /etc/systemd/system/wise-bridge.service
```

Paste the following configuration:

```ini
[Unit]
Description=WISE MQTT Gateway Bridge
After=network.target mosquitto.service

[Service]
ExecStart=/usr/bin/python3 -u /home/wisenet/gateway_bridge.py
WorkingDirectory=/home/wisenet
StandardOutput=inherit
StandardError=inherit
Restart=always
RestartSec=10
User=wisenet

[Install]
WantedBy=multi-user.target
```

### Enable and Start the Service

```bash
sudo systemctl daemon-reload
sudo systemctl enable wise-bridge.service
sudo systemctl start wise-bridge.service
```

- **Monitoring:** Do not run `python3 gateway_bridge.py` manually to monitor after boot, as they will fight for the port. Instead, monitor the active dataflow using:
  ```bash
  sudo journalctl -u wise-bridge.service -f
  ```
- **Troubleshooting:** If there are two gateway scripts fighting, stop the service before running it manually:
  ```bash
  sudo systemctl stop wise-bridge.service
  ```
  
## 5. Confirming the Mesh Data Pipeline

To verify that data is successfully flowing from the mesh network through the gateway and up to the cloud, use two terminal sessions on the Pi:

**Terminal 1 (Mesh Listener):**

```bash
mosquitto_sub -h localhost -v -t "mesh/#"
```

*(Wait until you see raw payload data arriving from the sensors before proceeding).*

**Terminal 2 (Gateway Bridge):**

```bash
python3 gateway_bridge.py
```

*(You should see `Cloud Connected: Success` followed by `[DATA] Forwarded for esp32_...` as the script routes the payloads).*

# ESP Sensor Node Setup
The Arduino IDE is used for the sensor node.
## Dependencies:
- [Esp32 Arduino](https://learn.adafruit.com/adafruit-esp32-feather-v2/arduino-ide-setup#install-esp32-board-support-package-3112219)
- [Adafruit LSM6DS Library](https://learn.adafruit.com/lsm6dsox-and-ism330dhc-6-dof-imu/arduino#library-installation-3048500)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

## Notes
- Make sure to change PPS_PIN in [ntp_client.h](firmware/ntp_client/ntp_client.h) if you aren't compiling for the esp32-s3.
- [influx_db_handler.h](firmware/ntp_client/influx_db_handler.h) skips the entire gateway.py router script and sends the data directly to Influx DB over http.  Enable it by defining USE_OLD.
