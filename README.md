# WiFi Weather Sensor that talks MQTT (SAP IoT Cloud Ready)

With `iot-weather-mqtt.ino` an **ESP8266** and a **DHT22** you can build a weather station with Internet.
So you build an Internet of Things (IoT) 🤓.
The sensor data are published by MQTT protocol.
The format corresponds to the expected format of the SAP IoT Cloud ☁️.
You can save the sensor data quickly and easily in the SAP Cloud and process it there.
If you want to get more and detailed information about the SAP Cloud, read my [SAP blog post](https://blogs.sap.com/2017/08/15/create-your-own-weather-station-with-sap-cloud-platform-internet-of-things/).



![Hardware](https://www.nkn-it.de/iot-weather-mqtt/hardware.jpg?v1)


## How does it work?

* The ESP8266 connects itself via WiFi and gets by DHCP an IP address
* The computer connects to the MQTT broker `iot.eclipse.org`
	* MQTT is explained in [Wikipedia](https://en.wikipedia.org/wiki/MQTT)
* Sensor data is sent as JSON (SAP IoT Cloud format) to the defined MQTT topic


## Requirements

You need:

* One ESP8266 - NodeMCU
    * I use the `NodeMCU Lua Lolin V3 Module`
* One DHT22 sensor
* Three LEDs
* A few cables and a Breadboard to connect everything

Costs about 15 EUR in Europe.


## Set-Up

The programming is done with the Arduino IDE. If you do not have it yet, install it.

Add in Preferences the "Additional Boards Manager URL":
`http://arduino.esp8266.com/stable/package_esp8266com_index.json`

![Preferences](https://www.nkn-it.de/iot-weather-mqtt/esp8266_url.jpg?v1)


### NodeMCU

Select the right board:

![NodeMCU](https://www.nkn-it.de/iot-weather-mqtt/nodemcu.jpg?v1)


### Libraries

Install the required libraries. Search for:

* DHT
* ESP8266WiFi
* PubSubClient
* ArduinoJson
* Adafruit Unified Sensor Driver

![Libraries](https://www.nkn-it.de/iot-weather-mqtt/libraries.jpg?v1)


### 🚨 Increase the MQTT library's memory limit 🚨

Open the `PubSubClient.h` file from the Arduino libraries folder.
Increasing `MQTT_MAX_PACKET_SIZE` to `256`.

On a Mac, this will be located at:
`~/Documents/Arduino/libraries/PubSubClient/src/PubSubClient.h`

## Wiring

![Wiring](https://www.nkn-it.de/iot-weather-mqtt/iot-weather-mqtt.jpg?v1)

Connect the DHT22 data pin to `D6`.

```
  _______
 /  ( )  \
 +-------+
 | [] [] |
 | [] [] |
 | [] [] |
 +-------+
  | | | |
  
   \ \ \ \__ Ground: Connect it to GND
    \ \ \___ Nothing
     \ \____ Data pin: Connect it to D6
      \_____ Positive pin: Connect it to 3v3
```

Connect `D0`, `D1`, `D2` each with one LED.


## Program

Open `iot-weather-mqtt.ino` with the Arduino IDE and adjust the configuration:

* `ssid`           : Your WiFi SSID
* `password`       : Your WiFi password
* `sap_iot_msg_id` : If you use the SAP IoT Cloud, the message type must be entered here
* `mqtt_topic`     : If you use the SAP IoT Cloud, the structure must be as follows: `iot/data/iotmms<ACCOUNT>/v1/<DEVICE>`

You find the data in the SAP Internet of Things Service Cockpit:

![sap_iot_msg_id](https://www.nkn-it.de/iot-weather-mqtt/sap_iot_msg_id.jpg?v1)

![Device ID](https://www.nkn-it.de/iot-weather-mqtt/device_id.jpg?v1)

![mqtt_topic](https://www.nkn-it.de/iot-weather-mqtt/mqtt_topic.jpg?v1)

Upload `iot-weather-mqtt.ino` to your ESP8266.

When everything is working, all LEDs light up:

* `D0` : Power on
* `D1` : WiFi connected
* `D2` : MQTT connected


Messages should appear on the MQTT Topic. Here's an example:

```
{
	"mode":"async",
	"messageType":"bd03144608253c6a4fca",
	"messages":[
		{"celcius":23.4,"fahrenheit":74.12,"humidity":67.59999}
	]
}
```
## Help 👍

If you have found a bug (English is not my mother tongue) or have any improvements, send me a pull request.