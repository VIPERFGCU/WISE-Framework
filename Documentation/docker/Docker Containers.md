# Creating Docker containers
## Prerequisites
[Docker](https://docs.docker.com/get-docker/)
## Using Docker Compose

## InfluxDB
```
docker run -d -it \
      --name myInflux \
      --restart "unless-stopped" \
      -p 8086:8086 \
      -v myInfluxVolume:/var/lib/influxdb2 \
      influxdb:latest
```


## Grafana
```
docker run -d -it \
      --name myGrafana \
      --restart "unless-stopped" \
      -p 3000:3000 \ 
      -volume /var/lib/grafana:/var/lib/grafana \
      -volume /etc/grafana:/etc/grafana \
      grafana/grafana-enterprise
```


### Configuring the datasource
Query Language - Flux

Http - http://{docker_container_name}:port Example: (http://influxdb:8086)

HTTP Headers

Header: "Authorization" 

Value: "Token YOUR_API_TOKEN_HERE"

Organization: "YOUR ORG ID"

## NodeRED
```
docker run -d -it \
      --name myNodeRED \
      --restart "unless-stopped" \
      -p 1880:1880 \
      -v node_red_data:/data \
      nodered/node-red
```


## Mosquitto
Make sure that the mounted files are already created on the host machine at:
```
docker run -d -it \
      --name myMosquitto \
      --restart "unless-stopped" \
      -p 1883:1883 \
      -p 9001:9001 \
      --volume /var/lib/mosquitto/config/mosquitto.conf:/mosquitto/config/mosquitto.conf \
      --volume /var/lib/mosquitto/config/mosquitto.pw:/mosquitto/config/mosquitto.pw \
      eclipse-mosquitto
```


https://community.home-assistant.io/t/help-with-mqtt-docker-setup/143759/4

## Docker Network
Add all containers to the same docker network
Example: 


`sudo docker network create myBridge`

`sudo docker network connect myBridge container1 container2 container3`...
