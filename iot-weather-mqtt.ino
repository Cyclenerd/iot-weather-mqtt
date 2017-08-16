/*
  With this program you can read temperature and humidity.
  The data is sent to an MQTT broker.
  The preparation of the data corresponds to the SAP IoT format.

  You need an ESP8266.
  Connect D0, D1, D2 each with one LED.
  Connect D6 to the DHT22 sensor.

  Open the PubSubClient.h file from the Arduino libraries folder.
  On a Mac, this will be located at:
  ~/Documents/Arduino/libraries/PubSubClient/src/PubSubClient.h
  Increasing MQTT_MAX_PACKET_SIZE to 256 
  
  More details: https://github.com/Cyclenerd/iot-weather-mqtt
 */

#include <DHT.h>          // https://github.com/adafruit/DHT-sensor-library and
                          // https://github.com/adafruit/Adafruit_Sensor
#include <ESP8266WiFi.h>  
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
const char* mqtt_server      = "iot.eclipse.org";
short unsigned int mqtt_port = 1883; // unencrypted

// Report every n Minutes
unsigned int report_every_min = 5; // 5 minutes

/***************************************************************************************
   End Configuration Section
 ***************************************************************************************/

#define DHTPIN D6     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22 // there are multiple kinds of DHT sensors

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// The setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  delay(1000); // wait 1 seconds

  // Initialize digital pins an output.
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);

  test_led();

  digitalWrite(D0, HIGH); // LED D0 on
  connect();
}

// The connect function makes a connection with the WiFi and the MQTT server
void connect() {
  digitalWrite(D1, LOW); // LED D1 off
  digitalWrite(D2, LOW); // LED D2 off
  
  // Start connecting to WiFi network
  Serial.println();
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
      Serial.println();
    }
    delay(500);
    Serial.print(".");
  }

  // Connected to WiFi
  digitalWrite(D1, HIGH); // LED D1 on
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup MQTT client
  client.setServer(mqtt_server, mqtt_port);

  Serial.println();
  Serial.print("Connecting to MQTT server ");
  Serial.println(mqtt_server);

  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("iot-weather-mqtt")) {
      digitalWrite(D2, HIGH); // LED D2 on
      Serial.println();
      Serial.println("MQTT connected");
      client.publish(mqtt_topic, "hello");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

// The test_led function tests the connected LEDs on D0, D1 and D2
void test_led() {
  digitalWrite(D0, HIGH);
  digitalWrite(D1, HIGH);
  digitalWrite(D2, HIGH);
  delay(5000); // wait 5 seconds
  digitalWrite(D0, LOW);
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
}

// The report function sends the data to the MQTT topic
void report(double humidity, double tempC, double tempF, double heatIndexC, double heatIndexF) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& rootObject = jsonBuffer.createObject();
  rootObject["mode"] = "async";
  rootObject["messageType"] = sap_iot_msg_id;
  JsonArray& messagesArray = rootObject.createNestedArray("messages");
  JsonObject& messageObject = messagesArray.createNestedObject();
  messageObject["celcius"] = tempC;
  messageObject["fahrenheit"] = tempF;
  messageObject["humidity"] = humidity;
  Serial.print("State: ");
  //rootObject.printTo(Serial);
  //Serial.println();

  char mqtt_buffer[256];
  rootObject.printTo(mqtt_buffer, sizeof(mqtt_buffer));
  Serial.println(mqtt_buffer);
  client.publish(mqtt_topic, mqtt_buffer);
}

unsigned int report_every_msec = report_every_min * 60 * 1000; // min to msec
unsigned int timeSinceLastRead = report_every_msec + 1;

// The loop function runs over and over again forever
void loop() {
  bool toReconnect = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (!client.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(client.state());
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  client.loop();
  
  // Report every n minutes (report_every_min -> report_every_msec)
  if(timeSinceLastRead > report_every_msec) {
    // Reading temperature or humidity takes about 250 ms!
    // Sensor readings may also be up to 2 seconds
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float c = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(c) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      return;
    }

    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(c, h, false);

    report(h, c, f, hic, hif);

    Serial.print("Wait ");
    Serial.print(report_every_min);
    Serial.println(" minutes...");
    timeSinceLastRead = 0;
  }
  delay(5000); // sleep 5 seconds
  timeSinceLastRead += 5000;

}
