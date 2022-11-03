/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>     // https://github.com/knolleary/pubsubclient

//--------------Para conectar desde wan
//  const char* ssid = "POCOPHONE";
//  const char* password = "12345678";
//  const char* mqtt_server = "mqtt1.iotcloud.studio";

//--------------Para conectar dentro de lan
  const char* ssid = "JOIN-IoTCloud";
  const char* password = "IoTcloud2019";
  const char* mqtt_server = "192.168.170.85";

//--------------Usuario y contraseña para mq
  const char* user = "guest";
  const char* pass = "guest";

//-------------------------- DHT --------------------------
#include "DHT.h"

#define DHTPIN 2              // pin data dht (pin D4) 
#define DHTTYPE DHT11         // modelo dht

DHT dht(DHTPIN, DHTTYPE);     // instanciar el DHT
//-------------------------- /DHT -------------------------

int prevt = 0;                // variable para saber si la temperatura cambio
int prevh = 0;                // variable para saber si la humedad cambio

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.print(ssid);
  Serial.println(" conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje llegado [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Intentando conexion MQTT...");
    // Create a random client ID
    // String clientId = "ESP8266Client-";
    // clientId += String(random(0xffff), HEX);
    String clientId = "NODEMCU";
    // Attempt to connect
    if (client.connect(clientId.c_str(),user,pass)) {
      Serial.println("Conectado");
      // Once connected, publish an announcement...
      client.publish("test/node/in/t", "Iniciando/t");
      client.publish("test/node/in/h", "Iniciando/h");
      // ... and resubscribe
      client.subscribe("test/node/out");
    } else {
      Serial.print("fallo conexion, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 seg");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  /*
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("test/node", msg);
  }*/

  unsigned long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    ++value;
    int t = dht.readTemperature();
    int h = dht.readHumidity();
    snprintf (msg, MSG_BUFFER_SIZE, "%ldTemperatura = %d°C", value, t);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("test/node/in/t", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%ldHumedad = %d%%", value, h);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("test/node/in/h", msg);
  }

  /*
  int t = dht.readTemperature();
  if (t != prevt){
    snprintf (msg, MSG_BUFFER_SIZE, "Temperatura = %d°C", t); 
    Serial.print("Mensaje publicado: ");
    Serial.println(msg);
    client.publish("test/node/in/t", msg);
    //client.publish("balcarce/oficina/tyh", msg);  
    prevt = t;
    }

  int h = dht.readHumidity();
  if (h != prevh){
    snprintf (msg, MSG_BUFFER_SIZE, "Humedad = %d%%", h); 
    Serial.print("Mensaje publicado: ");
    Serial.println(msg);
    client.publish("test/node/in/h", msg);
    //client.publish("balcarce/oficina/tyh", msg);
    prevh = h;
    }
    delay(2000);
  */
}
