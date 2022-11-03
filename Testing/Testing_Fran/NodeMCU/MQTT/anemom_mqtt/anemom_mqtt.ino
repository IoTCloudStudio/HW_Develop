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
#include <ArduinoJson.h>

#include <math.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

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

//--------------Variables fecha y hora
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hours = 0;
  uint8_t mins = 0;
  uint8_t seconds = 0;
  char* zone = "-03:00";

//--------------Variables para JSON
  const uint16_t ID = 11001;
  unsigned long LOG = 0;
  #define DATESIZE (25)
  char* DATETIME = 0;
  uint8_t CODIGO = 0;
  float DATO = 0;        //cambiar segun tipo de dato
  const uint16_t VER = 100;

  //uint8_t CODIGOTEM = 101;
  //uint8_t CODIGOHUM = 102;
  //uint8_t TEM = 0;
  //uint8_t HUM = 0;
  
  uint8_t CODIGOWINDS = 107;
  float WINDS = 0;

/*
//-------------------------- DHT --------------------------
#include "DHT.h"

#define DHTPIN 2              // pin data dht (pin D4) 
#define DHTTYPE DHT11         // modelo dht

DHT dht(DHTPIN, DHTTYPE);     // instanciar el DHT

int prevt = 0;                // variable para saber si la temperatura cambio
int prevh = 0;                // variable para saber si la humedad cambio
//-------------------------- /DHT -------------------------
*/

//-------------------------- WINDS --------------------------
#define WindSensorPin (2) // The pin location of the anemometer sensor

volatile unsigned long Rotations; // cup rotation counter used in interrupt routine
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine

float WindSpeed; // speed miles per hour
//-------------------------- /WINDS -------------------------

//--------------Instanciar objetos
WiFiClient espClient;           //conexion wifi
PubSubClient client(espClient); //conexion mqtt

WiFiUDP ntpUDP;                 //conexion udp
NTPClient timeClient(ntpUDP, "pool.ntp.org",-10800);   //conexion ntp
//--------------/Instanciar objetos

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

  timeClient.begin();
  
  year = 2021;
  month = 02;
  day = 23;
  hours = 14;
  
  DATETIME = "2021-02-";
  
  //----DEBUG----
  Serial.begin(115200);
  //----/DEBUG---

  /*
  //-----DHT-----
  dht.begin();
  //-----/DHT----
  */
  
  //-----WINDS-----
  pinMode(WindSensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING);
  Serial.println("Davis Wind Speed Test");
  Serial.println("Rotations\tMPH");
  //-----/WINDS----
  
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  setup_wifi();
  
  //-----MQTT------
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //-----/MQTT-----
  
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  //-------DHT------
/*int t = dht.readTemperature();
  int h = dht.readHumidity();
  TEM = t;
  HUM = h;  */
  /*
  TEM = dht.readTemperature();
  HUM = dht.readHumidity();
  */
  //-------/DHT-----
  
  //-------WINDS------
  Rotations = 0; // Set Rotations count to 0 ready for calculations
  sei(); // Enables interrupts
  delay (3000); // Wait 3 seconds to average
  cli(); // Disable interrupts
  WindSpeed = Rotations * 0.75 * 1.61;
  Serial.print(Rotations); Serial.print("\t\t");
  Serial.println(WindSpeed);
  WINDS = WindSpeed
  //-------/WINDS-----
  
  /*  deprecated, version 5
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
 
  JSONencoder["I"] = 2153;
  JSONencoder["C"] = 10;
  JSONencoder["D"] = t;
  JSONencoder["V"] = 1;
 
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);
  */
  timeClient.update();
  
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime); 
  
  hours = timeClient.getHours();
  Serial.print("Hour: ");
  Serial.println(hours);  
  
  mins = timeClient.getMinutes();
  Serial.print("Minutes: ");
  Serial.println(mins); 
   
  seconds = timeClient.getSeconds();
  Serial.print("Seconds: ");
  Serial.println(seconds);  
   
  LOG++;
  //------DHT------
  //CODIGO = CODIGOTEM;
  //DATO = TEM;
  //------/DHT-----

  //------WINDS-----
  CODIGO = CODIGOWINDS;
  DATO = WINDS;
  //------WINDS-----
  snprintf (DATETIME, DATESIZE, "%u-%u-%uT%u:%u:%u%s", year, month, day, hours, mins, seconds, zone);
  String Date = String(year) + "-" + String(month) + "-" + String(day) + "T" + formattedTime + zone;
  Serial.println(DATETIME);
  Serial.println(Date);

  //------GUARDAR LOG-------
  
  //------/GUARDAR LOG------
  
  //------ARMADO JSON-------
  StaticJsonDocument<256> JSONdoc;
  JSONdoc["I"] = ID;
  JSONdoc["L"] = LOG;
  JSONdoc["F"] = Date;
  JSONdoc["C"] = CODIGO;
  JSONdoc["D"] = DATO;
  JSONdoc["V"] = VER;
  char JSONmessageBuffer[256];
  serializeJson(JSONdoc,JSONmessageBuffer);
  Serial.println(JSONmessageBuffer);
  //------/ARMADO JSON------
  
  if (client.publish("test", JSONmessageBuffer) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
 
  client.loop();
  Serial.println("-------------");
 
  delay(10000);
  
  
  client.loop();

  
  /*            EJEMPLO QUE VIENE DE FABRICA
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("test/node", msg);
  }*/


  /*
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
  */

  /*             MANDAR TEMPERATURA Y HUMEDAD CUANDO CAMBIA VALOR
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

void isr_rotation () {

if ((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact.
Rotations++;
ContactBounceTime = millis();
}

}
