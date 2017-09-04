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

// Report every n Minutes
const unsigned int report_every_min = 5; // 5 minutes

// Publish Uptime to MQTT Topic
const bool publish_uptime = true; // yes = true, no = false

/***************************************************************************************
   End Configuration Section
 ***************************************************************************************/


#define DHTPIN D6     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22 // there are multiple kinds of DHT sensors

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long time_msec;
unsigned long last_report_msec;
const unsigned int report_every_msec = report_every_min * 60 * 1000; // min to msec
const char* random_client = __DATE__ " " __TIME__;

/***************************************************************************************
 * Helpers 
 ***************************************************************************************/
 
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

// The uptime function keeps a uptime counter that will survive the 50 day timer rollover
// https://www.arduino.cc/en/Reference/Millis
unsigned long uptime_day       = 0;
unsigned int  uptime_hour      = 0;
unsigned int  uptime_min       = 0;
unsigned int  uptime_sec       = 0;
unsigned int  uptime_rollover  = 0;
bool          uptime_high_msec = false;
void uptime() {
  // Making Note of an expected rollover
  if (millis() >= 3000000000) {
    uptime_high_msec = true;
  }

  // Making note of actual rollover
  if (millis() <= 100000 && uptime_high_msec) {
    uptime_rollover++;
    uptime_high_msec = false;
  }

  unsigned long time_sec = millis() / 1000;
  uptime_sec  = time_sec % 60;
  uptime_min  = (time_sec / 60) % 60;
  uptime_hour = (time_sec / (60 * 60)) % 24;
  uptime_day  = (uptime_rollover * 50) + (time_sec / (60 * 60 * 24)); // First portion takes care of a rollover [around 50 days]
}

// The connect function makes a connection with the WiFi and the MQTT server
void connect() {
  digitalWrite(D1, LOW); // LED D1 off
  digitalWrite(D2, LOW); // LED D2 off

  // Start connecting to WiFi network
  if (WiFi.status() != WL_CONNECTED) {
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
  }
  // Connected to WiFi
  digitalWrite(D1, HIGH); // LED D1 on
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup MQTT client
  client.setServer(mqtt_server, mqtt_port);
  //client.setCallback(callback);

  Serial.println();
  Serial.print("Connecting to MQTT server ");
  Serial.println(mqtt_server);

  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect(random_client)) {
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


/***************************************************************************************
 * Let's start 
 ***************************************************************************************/

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

  Serial.print("MQTT max packet size: ");
  Serial.println(MQTT_MAX_PACKET_SIZE);
  Serial.print("MQTT client ID: ");
  Serial.println(random_client);
  
  connect();
}

// The loop function runs over and over again forever
void loop() {
  uptime(); // get uptime since the board began running
  bool reconnect = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    reconnect = true;
  }
  if (!client.connected()) {
    Serial.print("Disconnected from MQTT, rc=");
    Serial.println(client.state());
    reconnect = true;
  }
  if (reconnect) {
    connect();
  }

  // Status report ever n minutes
  // millis() will overflow (go back to zero), after approximately 50 days
  if (last_report_msec == 0 || millis() - last_report_msec >= report_every_msec || millis() < last_report_msec) {
    // Uptime N days, HH:MM:SS
    String uptime_string = "Uptime: " +
                           String(uptime_day) + " days, " +
                           (uptime_hour < 10 ? "0" : "") + String(uptime_hour) + ":" +
                           (uptime_min < 10 ? "0" : "") + String(uptime_min) + ":" +
                           (uptime_sec < 10 ? "0" : "") + String(uptime_sec);
    Serial.print(uptime_string);
    Serial.println();
    // Publish uptime
    if (publish_uptime) {
      client.publish(mqtt_topic, uptime_string.c_str());
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
      delay(2000); // sleep 2 seconds
      return;
    }

    // Compute heat index in Fahrenheit (the default)
    float heat_index_f = dht.computeHeatIndex(temp_f, humidity);
    // Compute heat index in Celsius (isFahreheit = false)
    float heat_index_c = dht.computeHeatIndex(temp_c, humidity, false);

    report(humidity, temp_c, temp_f, heat_index_c, heat_index_f);

    Serial.print("Wait ");
    Serial.print(report_every_min);
    Serial.println(" minutes...");
    last_report_msec = millis();
  }

  client.loop();
}
