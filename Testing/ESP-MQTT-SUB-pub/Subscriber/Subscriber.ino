#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid ="AntiGORILAS"; //replace this with your wifi  name
const char* password ="entreri0s*4*3*6*1"; //replace with your wifi password
const char* broker = "testmqtt.iotcloud.studio"; //replace this with IP address of machine 
//on which broker is installed
const int port = 51883;
const char* mqttUser = "user";
const char* mqttPassword = "user";

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

  client.setServer(broker, port);
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
  Serial.println("Subscribing topic test:");
  client.subscribe("test");//topic name="abc"

}

void loop() {
  client.loop();
}