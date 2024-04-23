# Mosquitto Setup

mosquitto.conf example:
```
allow_anonymous false
listener 1883
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.logs true
password_file /path/to/mosquitto.pw
```


mosquitto.pw example:
```
username1:password1
username2:password2
username3:password3
...
```


After populating the password file, the file can be encrypted using the command:

`mosquitto_passwd -U mosquitto.pw`

More info can be found [here](http://www.steves-internet-guide.com/mqtt-username-password-example/)

# Node-RED

Configuring mqtt workflow using the default port value of 1883 for Mosquitto:

[Connecting MQTT Broker (Mosquitto)](https://cookbook.nodered.org/mqtt/connect-to-broker)

Before configuring MQTT input or output, you need an mqtt-broker node. For this, we use Mosquitto.



You can configure an MQTT in or MQTT out 
## MQTT In

Server: Mosquitto_IP:Mosquitto_Port (1.2.3.4:1883)
Action: Subscribe to a single topic
Topic: myTopic/toNodeRed
QoS: 0
[Node-RED Documentation](https://cookbook.nodered.org/)

## MQTT Out

Server 
