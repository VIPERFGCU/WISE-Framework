
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

## 6. Sensor Flashing Guide

Ensure your laptop is simulating the cloud server and you are connected to the Pi's hotspot.

### Base ESP32

- Attach the USB to WSL:
  ```bash
  usbipd bind --busid 2-4
  ```
- Upload and monitor:
  ```bash
  pio run -t upload -t monitor
  ```

### ESP32-S3

The ESP32-S3 often hides its serial port until it is forced into "Download Mode".

1. Hold down the BOOT (0) button on the S3 board.
2. Click RESET (or plug it in) while holding BOOT, then release BOOT.
3. Pass it back to WSL via PowerShell:
   ```powershell
   usbipd attach --wsl --busid <YOUR_BUS_ID>
   ```
4. Upload and monitor:
   ```bash
   pio run -e esp32-s3-devkitc-1 -t upload -t monitor
   ```

## 7. Over-The-Air (OTA) Updates [WIP]

The following outlines the current progress and testing workflow for OTA mesh updates via HTTP. Ensure OTA is enabled for HTTP in the sensor configuration.

### Phase 1: Hardware & Cleanup

Before attempting an OTA update, clear any hanging processes and reset the broker:

```bash
sudo pkill -f python3
sudo systemctl restart mosquitto
```

### Phase 2: Root Node OTA (Self-Update)

- Compile binary (do not upload):
  ```bash
  pio run -e esp32dev
  ```
- Move file to Pi:
  ```bash
  scp .pio/build/esp32dev/firmware.bin wisenet@wisenet-gateway:~/update.bin
  ```
- Start web server (Terminal 3):
  ```bash
  python3 -m http.server 8000
  ```
- Trigger command (Terminal 4) — tell the Root Node (IP `10.10.10.5`) to download from the Pi (IP `10.10.10.1`):
  ```bash
  mosquitto_pub -h localhost -t "mesh/ota/command" -m "http://10.10.10.1:8000/update.bin"
  ```

**Expected Result:** Terminal 3 will show `GET /update.bin 200 OK`. Terminal 2 (Bridge) will show `[OTA] Received Command!` and later `[DATA] ... (v2.0 OTA SUCCESS)`. In Terminal 1, the Root node will drop offline, then return with an updated version number.

### Phase 3: Child Propagation (Infect the Mesh)

- Trigger broadcast (Terminal 4):
  ```bash
  mosquitto_pub -h localhost -t "mesh/ota/propagate" -m "GO"
  ```

**Expected Result:** Terminal 1 will show the Root node sending data chunks. The Child node will drop offline, install the firmware, reboot, and return with the updated version number.

### Unresolved OTA Architecture Questions

- **Leg 1 (Self-Update):** Can the Root grab a file from the server and update itself reliably?
- **Leg 2 (Propagation):** Can the Root grab a different file (the S3 firmware) and pass it to the children instead of installing it on itself?
- How to trigger the OTA command directly from the Docker backend/frontend instead of manually via Pi terminals.
- How to dynamically build new firmware and push it through the CI/CD container pipeline.
- Strategies for tracking firmware versions and defining the rollback retention period for previous firmware binaries.

