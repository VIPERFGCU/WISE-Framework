# Creating Docker containers
### Prerequisites
[Docker](https://docs.docker.com/get-docker/)
## Using Docker Compose

## InfluxDB
```
docker run -d -it \
      --name yourInfluxName \
      --restart "unless-stopped" \
      -p 8086:8086 \
      -v myInfluxVolume:/var/lib/influxdb2 \
      influxdb:latest
```


## Grafana
```
docker run -d -it \
      --name yourGrafanaName \
      --restart "unless-stopped" \
      -p 3000:3000 \ 
      -v grafana_data:/var/lib/grafana \
      -v grafana_config:/etc/grafana \
      grafana/grafana-enterprise
```

If using bind mounts with Grafana, file permissions must be set up properly.

the workaround is to:

    manually create an empty grafana.ini in the volume of local host that will be mapped as bind volume to /etc/grafana/grafana.ini

    manually create empty directories in below volumes of local host that will be mapped as bind volume to:
    etc/grafana/provisioning/datasources
    etc/grafana/provisioning/plugins
    etc/grafana/provisioning/notifiers
    etc/grafana/provisioning/dashboards

    set all above manually created file and directories to user=472 group=0
Taken from [here](https://github.com/grafana/grafana/issues/51860#issuecomment-1178651261)


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
      --name yourNodeREDname \
      --restart "unless-stopped" \
      -p 1880:1880 \
      -v node_red_data:/data \
      nodered/node-red
```


## Mosquitto
```
docker run -d -it \
      --name yourMosquittoName \
      --restart "unless-stopped" \
      -p 1883:1883 \
      -p 9001:9001 \
      --volume /var/lib/mosquitto/config/mosquitto.conf:/mosquitto/config/mosquitto.conf \
      --volume /var/lib/mosquitto/config/mosquitto.pw:/mosquitto/config/mosquitto.pw \
      eclipse-mosquitto
```
Make sure that the mounted files are already created on the host machine at the locations specified in the docker run command or docker-compose file

Addititional info [here](https://community.home-assistant.io/t/help-with-mqtt-docker-setup/143759/4)

## Docker Network
Add all containers to the same docker network
Example: 


`sudo docker network create myBridge`

`sudo docker network connect myBridge container1`

`sudo docker network connect myBridge container2`

`sudo docker network connect myBridge container3`

`sudo docker network connect myBridge container4`
