#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "IoT";
const char* password = "IoTcloud2019";
const char* mqtt_server_domain = "192.168.170.84"; // Remoto: "testmqtt.iotcloud.studio";
const long mqtt_server_port = 1883;// Remoto: 51883;
const char* mqttUser = "user";
const char* mqttPassword = "user";                           // We'll use the prefix to describe a 'family' of devices.
const char* subscribetopic = "ALARM/PUERTA1";     // Topics that we will subscribe to within that family.
const char* topic = "ALARM/#";     // Topics that we will subscribe to within that family.
String deviceId = "1";
int deviceLog = 0;
String deviceDesciption = "SensorPuerta1";
const char* TIME_SERVER = "pool.ntp.org";
int myTimeZone = ARG; // change this to your time zone (see in timezone.h)

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message received in topic: ");
  Serial.print(topic);
  Serial.print("   length is:");
  Serial.println(length);

  Serial.print("Data Received From Broker:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");
  Serial.println();

}

void setup() {

  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    yield();
  }
  Serial.println("Connected to the WiFi network");

  client.setServer(mqtt_server_domain, mqtt_server_port);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client"))
    {

      Serial.println("connected to MQTT broker");

    }
    else
    {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(500);

    }
  }

  
  Serial.println("ESP8266 AS SUBSCRIBER");
  client.subscribe(topic);//topic name="abc"

}

void loop() {
  client.loop();
}
