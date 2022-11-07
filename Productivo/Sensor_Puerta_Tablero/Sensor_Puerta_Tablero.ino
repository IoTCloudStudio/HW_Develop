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
char code[32] = "";

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<300> doc;
  if (topic = "ALARM/PUERTA1"){
    deserializeJson(doc, payload, length);
    strlcpy(code, doc["C"] | "default", sizeof(code));
    if (strcmp (code,"E301") == 0){
      Serial.println("PUERTA 1 sonorizando");
      Serial.println("-----------------------");
      Serial.println();
    }
  }
  else   if (topic = "ALARM/PUERTA2"){
    deserializeJson(doc, payload, length);
    strlcpy(code, doc["C"] | "default", sizeof(code));
    if (strcmp (code,"E301") == 0){
      Serial.println("PUERTA 2 sonorizando");
      Serial.println("-----------------------");
      Serial.println();
    }
  }
  else   if (topic = "ALARM/PUERTA3"){
    deserializeJson(doc, payload, length);
    strlcpy(code, doc["C"] | "default", sizeof(code));
    if (strcmp (code,"E301") == 0){
      Serial.println("PUERTA 3 sonorizando");
      Serial.println("-----------------------");
      Serial.println();
    }
  }

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
