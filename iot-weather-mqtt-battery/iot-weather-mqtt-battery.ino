/*
  With this program you can read temperature and humidity.
  The data is sent to an MQTT broker.
  The preparation of the data corresponds to the SAP IoT format.

  =====================================================================================
  Optimized for battery operation. Device goes into Deep Sleep.
  According to the ESP8266 SDK, you can sleep max. for about ~71 minutes.
  =====================================================================================

  You need an ESP8266.
  Connect D0 with RST.
  Connect D6 to the DHT22 sensor.

  Open the PubSubClient.h file from the Arduino libraries folder.
  On a Mac, this will be located at:
  ~/Documents/Arduino/libraries/PubSubClient/src/PubSubClient.h
  Increasing MQTT_MAX_PACKET_SIZE to 256

  More details: https://github.com/Cyclenerd/iot-weather-mqtt
*/

#include <ESP8266WiFi.h>  // https://github.com/esp8266/Arduino
#include <DHT.h>          // https://github.com/adafruit/DHT-sensor-library and
                          // https://github.com/adafruit/Adafruit_Sensor
#include <PubSubClient.h> // https://pubsubclient.knolleary.net/
#include <ArduinoJson.h>  // https://bblanchon.github.io/ArduinoJson/


/***************************************************************************************
   Configuration Section
 ***************************************************************************************/

// WiFi SSID Name
const char* ssid     = "YOUR-WIFI-SSID";
// WiFi Password
const char* password = "YOUR-WIFI-PASSWORD";

// SAP IoT Message ID
const char* sap_iot_msg_id = "<SAP-MESSAGE-ID>";

// MQTT Topic
const char* mqtt_topic = "iot/data/iotmms<ACCOUNT>/v1/<DEVICE>";


// MQTT Server
const char* mqtt_server            = "iot.eclipse.org";
const short unsigned int mqtt_port = 1883; // unencrypted

// MQTT Server
const char* mqtt_server            = "iot.eclipse.org";
const short unsigned int mqtt_port = 1883; // unencrypted

// Report every n Minutes
const short unsigned int report_every_min = 5; // 5 minutes

/***************************************************************************************
   End Configuration Section
 ***************************************************************************************/


#define DHTPIN D6     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22 // there are multiple kinds of DHT sensors

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

const unsigned int sleep_time = report_every_min * 60 * 1000000;
const short unsigned int max_connection_attempts = 50;
unsigned short int connection_attempts = 0;

/***************************************************************************************
   Helpers
 ***************************************************************************************/

// The connect function makes a connection with the WiFi and the MQTT server
void connect() {

  // Start connecting to WiFi network
  if (WiFi.status() != WL_CONNECTED) {
    // Check the maximum WiFi connection attempts
    if (connection_attempts > max_connection_attempts) {
      Serial.println("Maximum connection attempts for WiFi reached. Going to sleep now.");
      deep_sleep(); // Goodnight
    }

    Serial.print("Connecting to WiFi name ");
    Serial.println(ssid);

    // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      if (WiFi.status() == WL_CONNECT_FAILED) {
        Serial.println("Failed to connect to WIFI. Please verify credentials!");
      }
      connection_attempts++; // Count connection attempts
      delay(500);
      Serial.print(".");
    }
    Serial.println();
  }
  // Connected to WiFi
  connection_attempts = 0; // Reset
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup MQTT client
  Serial.print("Connecting to MQTT server ");
  Serial.println(mqtt_server);
  client.setServer(mqtt_server, mqtt_port);

  // Loop until we're reconnected
  while (!client.connected()) {
    // Check the maximum MQTT connection attempts
    if (connection_attempts > max_connection_attempts) {
      Serial.println("Maximum connection attempts for MQTT reached. Going to sleep now.");
      deep_sleep();
    }

    // Attempt to connect
    if (client.connect("iot-weather-mqtt")) {
      connection_attempts = 0;
      Serial.println("MQTT connected");
      client.publish(mqtt_topic, "hello");
    } else {
      connection_attempts++;
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(1000); // Wait 1 seconds before retrying
    }
  }
}

// The report function sends the data to the MQTT topic
void report(double humidity, double temp_c, double temp_f, double heat_index_c, double heat_index_f) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& rootObject = jsonBuffer.createObject();
  rootObject["mode"] = "async";
  rootObject["messageType"] = sap_iot_msg_id;
  JsonArray& messagesArray = rootObject.createNestedArray("messages");
  JsonObject& messageObject = messagesArray.createNestedObject();
  messageObject["celcius"] = temp_c;
  messageObject["fahrenheit"] = temp_f;
  messageObject["humidity"] = humidity;
  Serial.print("State: ");
  //rootObject.printTo(Serial);
  //Serial.println();

  char mqtt_buffer[256];
  rootObject.printTo(mqtt_buffer, sizeof(mqtt_buffer));
  Serial.println(mqtt_buffer);
  // Publish
  client.publish(mqtt_topic, mqtt_buffer);
}

// The deep_sleep function sends the device into the deep sleep.
void deep_sleep() {
  Serial.print("Going into deep sleep for ");
  Serial.print(report_every_min);
  Serial.println(" minutes...");
  ESP.deepSleep(sleep_time);
}


/***************************************************************************************
   Let's start
 ***************************************************************************************/

// The setup function runs once when you press reset or power the board (after deep sleep)
void setup() {
  Serial.begin(115200);
  Serial.println();
  connect();
}

// The loop function runs over and over again
short unsigned int sensor_attempts = 0;
void loop() {
  // Check the maximum sensor reading attempts
  if (sensor_attempts >= 3) {
    deep_sleep();
  }

  // Reading temperature or humidity takes about 250 ms!
  // Sensor readings may also be up to 2 seconds
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temp_c = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float temp_f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temp_c) || isnan(temp_f)) {
    Serial.println("Failed to read from DHT sensor!");
    sensor_attempts++;
    delay(2000); // sleep 2 seconds
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float heat_index_f = dht.computeHeatIndex(temp_f, humidity);
  // Compute heat index in Celsius (isFahreheit = false)
  float heat_index_c = dht.computeHeatIndex(temp_c, humidity, false);

  report(humidity, temp_c, temp_f, heat_index_c, heat_index_f);

  deep_sleep();
}
