#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const int ledState = 16;
const int ledPuerta1 = 5;
const int ledPuerta2 = 4;
const int ledPuerta3 = 0;
int BUTTON_PIN = 13;

const byte TONE_PIN = 12;
const int ALARM_BEEP_1 = 4186;
const int ALARM_BEEP_2 = 4699;

const int ALARM_TONE_LENGTH = 200;
const int ALARM_TONE_PAUSE = 800;
const int ALARM_TONE_REPEAT = 6;
boolean alarm = false;

const char* ssid = "CMR";
const char* password = "Rosario2020";
const char* mqtt_server_domain = "192.168.2.93";
const long mqtt_server_port = 1883;
//const char* ssid = "IoT";
//const char* password = "IoTcloud2019";
//const char* mqtt_server_domain = "192.168.170.84"; // Remoto: "testmqtt.iotcloud.studio";//
//const long mqtt_server_port = 1883;// Remoto: 51883;
const char* mqttUser = "user";
const char* mqttPassword = "user";                           // We'll use the prefix to describe a 'family' of devices.
const char* subscribetopic = "ALARM/#";     // Topics that we will subscribe to within that family.
const char* topic = "ALARM/#";     // Topics that we will subscribe to within that family.
String deviceId = "0";
int deviceLog = 0;
String deviceDesciption = "SensorPuerta1";
const char* TIME_SERVER = "pool.ntp.org";
char code[32] = "";

WiFiClient espClient;
PubSubClient client(espClient);

void alarmSound() {
  static unsigned long next = millis();
  static byte count = 0;
  if (millis() > next) {
    next += ALARM_TONE_LENGTH;
    count++;
    if (count == ALARM_TONE_REPEAT) {
      next += ALARM_TONE_PAUSE;
      count = 0;
    }
    tone(TONE_PIN, (count % 2) ? ALARM_BEEP_1 : ALARM_BEEP_2, ALARM_TONE_LENGTH);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<300> doc;
  Serial.println(topic);
  if (strcmp (topic,"ALARM/PUERTA1") == 0){
    deserializeJson(doc, payload, length);
    strlcpy(code, doc["C"] | "default", sizeof(code));
    if (strcmp (code,"E301") == 0){
      alarm = true;
      digitalWrite(ledPuerta1, 1);
      Serial.println("PUERTA 1 sonorizando");
      Serial.println("-----------------------");
      Serial.println();
      
    }
    else if (strcmp (code,"R301") == 0){
      digitalWrite(ledPuerta1, 0);
      Serial.println("PUERTA 1 cerrada");
      Serial.println("-----------------------");
      Serial.println();
    }
  }
  else   if (strcmp (topic,"ALARM/PUERTA2") == 0){
    deserializeJson(doc, payload, length);
    strlcpy(code, doc["C"] | "default", sizeof(code));
    if (strcmp (code,"E301") == 0){
      alarm = true;
      digitalWrite(ledPuerta2, 1);
      Serial.println("PUERTA 2 sonorizando");
      Serial.println("-----------------------");
      Serial.println();
    }
    else if (strcmp (code,"R301") == 0){
      digitalWrite(ledPuerta2, 0);
      Serial.println("PUERTA 2 cerrada");
      Serial.println("-----------------------");
      Serial.println();
    }
  }
  else   if (strcmp (topic,"ALARM/PUERTA3") == 0){
    deserializeJson(doc, payload, length);
    strlcpy(code, doc["C"] | "default", sizeof(code));
    if (strcmp (code,"E301") == 0){
      alarm = true;
      digitalWrite(ledPuerta3, 1);
      Serial.println("PUERTA 3 sonorizando");
      Serial.println("-----------------------");
      Serial.println();
    }
    else if (strcmp (code,"R301") == 0){
      digitalWrite(ledPuerta3, 0);
      Serial.println("PUERTA 3 cerrada");
      Serial.println("-----------------------");
      Serial.println();
    }
  }

}

void setup() {

  Serial.begin(115200);
  pinMode(ledState, OUTPUT);
  pinMode(ledPuerta1, OUTPUT);
  pinMode(ledPuerta2, OUTPUT);
  pinMode(ledPuerta3, OUTPUT);
  pinMode(BUTTON_PIN,INPUT);
  pinMode(TONE_PIN, OUTPUT);
  digitalWrite(ledState, 0);
  digitalWrite(ledPuerta1, 0);
  digitalWrite(ledPuerta2, 0);
  digitalWrite(ledPuerta3, 0);
  

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    yield();
  }
  Serial.println("Connected to the WiFi network");
  digitalWrite(ledState, 1);

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
  byte valor = digitalRead(BUTTON_PIN);
  if(valor==HIGH )
     {
      alarm = false;
      Serial.println("--Presionado--");
      digitalWrite(TONE_PIN, LOW);
     }
  
   if (alarm) {
    //alarmSound();
    digitalWrite(TONE_PIN, HIGH);
  }
}
