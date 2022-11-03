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

#include <ESP8266WiFi.h>      // Libreria Wifi esp8266            // 
#include <PubSubClient.h>     // Libreria mqtt                    // https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h>      // Libreria armado de Json          // 
#include <NTPClient.h>        // Libreria NTP                     // 
#include <WiFiUdp.h>          // Libreria UDP necesaria para NTP  // 

//--------------Para conectar desde wan
//  const char* ssid = "POCOPHONE";
//  const char* password = "12345678";
//  const char* mqtt_server = "mqtt1.iotcloud.studio";

//--------------Para conectar dentro de lan
  const char* ssid = "JOIN-IoTCloud";
  const char* password = "IoTcloud2019";
  const char* mqtt_server = "192.168.170.85";

//--------------Usuario y contraseña para mqtt
  const char* user = "guest";
  const char* pass = "guest";

//--------------Variables para JSON
  const uint32_t  ID = 11001;       // ID dispositivo
        uint32_t  LOG = 0;          // Log ID incremental con cada envio
        uint32_t  TIME = 0;         // Epoch time obtenido del NTP
        uint8_t   CODIGO = 0;       // Codigo operacion
        uint8_t   DATO = 0;         // Cambiar segun tipo de dato
  const uint16_t  VERSION = 100;    // Version 0.1.0

  const uint8_t codigohb = 1;       // Codigo a mandar cuando se envia señal de vida
  uint16_t hb = 0;                  // Dato a enviar de señal de vida (incremental desde reset)
  const uint8_t codigot = 101;      // Codigo a mandar cuando se envia temperatura
  const uint8_t codigoh = 102;      // Codigo a mandar cuando se envia humedad
  uint8_t t = 0;                    // Dato a enviar de temperatura
  uint8_t h = 0;                    // Dato a enviar de humedad

//-------------------------- DHT --------------------------
#include "DHT.h"

#define DHTPIN 2              // pin data dht (pin D4) 
#define DHTTYPE DHT11         // modelo dht

DHT dht(DHTPIN, DHTTYPE);     // instanciar el DHT

int prevt = 0;                // variable para saber si la temperatura cambio
int prevh = 0;                // variable para saber si la humedad cambio
//-------------------------- /DHT -------------------------

//--------------Instanciar objetos
WiFiClient espClient;           //conexion wifi
PubSubClient client(espClient); //conexion mqtt

WiFiUDP ntpUDP;                 //conexion udp para ntp
NTPClient timeClient(ntpUDP, "pool.ntp.org",-10800);   //conexion ntp
//--------------/Instanciar objetos

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

//_______________________________________________________________________________________
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
//_______________________________________________________________________________________
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje llegado [");
  Serial.print(topic);
  Serial.print("]: ");
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
//_______________________________________________________________________________________
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Intentando conexion MQTT...");
    // Create a random client ID
    String clientId = "NODEMCU-";
    clientId += String(ID);
    // Attempt to connect
    if (client.connect(clientId.c_str(),user,pass)) {
      Serial.println("Conectado");
      // Once connected, publish an announcement...
      client.publish("", "Iniciando/t");
      // ... and resubscribe
      client.subscribe("send");
    } else {
      Serial.print("fallo conexion, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 seg");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//_______________________________________________________________________________________
void setup() {
  
  //---DEBUG-----
  Serial.begin(115200);
  //---/DEBUG----
  
  //---PINES-----
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  //---/PINES----

  //---MQTT------
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //---/MQTT-----

  //---WIFI------
  setup_wifi();
  //---/WIFI-----
  
  //---NTP-------
  timeClient.begin();
  //---/NTP------
  
  //---DHT-------
  dht.begin();
  //---/DHT------
  
}
//_______________________________________________________________________________________
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // ------- MANDAR MENSAJE CADA CIERTO TIEMPO
  unsigned long now = millis();
  if (now - lastMsg > 30000) {
    lastMsg = now;    
    datasend(codigohb,hb);
    hb++;
  }

  // --------- MANDAR TEMPERATURA Y HUMEDAD CUANDO CAMBIA VALOR
  t = dht.readTemperature();
  if (t != prevt){
    datasend(codigot,t);
    prevt = t;
    }

  h = dht.readHumidity();
  if (h != prevh){
    datasend(codigoh,h);
    prevh = h;
    }
    
  delay(2000);
    
}
//_______________________________________________________________________________________
boolean datasend(uint8_t cod, uint8_t dato){

  timeClient.update();
  LOG++;
  TIME = timeClient.getEpochTime();
  Serial.println(TIME);
  CODIGO = cod;
  DATO = dato;

  //------GUARDAR LOG-------
  
  //------/GUARDAR LOG------
  
  //------ARMADO JSON-------
  StaticJsonDocument<256> JSONdoc;
  JSONdoc["I"] = ID;
  JSONdoc["L"] = LOG;
  JSONdoc["T"] = TIME;
  JSONdoc["C"] = CODIGO;
  if(DATO){
    JSONdoc["D"] = DATO;}
  JSONdoc["V"] = VERSION;
  char JSONmessageBuffer[256];
  serializeJson(JSONdoc,JSONmessageBuffer);
  Serial.println(JSONmessageBuffer);
  //------/ARMADO JSON------

  //------ENVIO MQTT--------
  if (client.publish("test", JSONmessageBuffer) == true) {
    Serial.println("Mensaje enviado");
    Serial.println("-----------------------------------");
    return (true);
    } else {
    Serial.println("Error enviando mensaje");
    Serial.println("///////////////////////////////////");
    return (false);
    }
  //------/ENVIO MQTT-------
  
}
