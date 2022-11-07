#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "timezone.h"
#include <ArduinoJson.h>

// Set GPIOs for LED and reedswitch
const int reedSwitch = 4;
const int led = 2; //optional
 
// Detects whenever the door changed state
bool changeState = false;

// Holds reedswitch state (1=opened, 0=close)
bool state;
String doorState;


// Auxiliary variables (it will only detect changes that are 1500 milliseconds apart)
unsigned long previousMillis = 0;
unsigned long lastMillis = 0; 
const long interval_sensor = 1500;
const long interval_hb = 60000;

const char* ssid = "IoT";
const char* password = "IoTcloud2019";
const char* mqtt_server_domain = "192.168.170.84"; // Remoto: "testmqtt.iotcloud.studio";
const long mqtt_server_port = 1883;// Remoto: 51883;
const char* mqttUser = "user";
const char* mqttPassword = "user";                           // We'll use the prefix to describe a 'family' of devices.
const char* subscribetopic = "ALARM/PUERTA1";     // Topics that we will subscribe to within that family.
const char* topic = "ALARM/PUERTA1";     // Topics that we will subscribe to within that family.
String deviceId = "1";
int deviceLog = 0;
String deviceDesciption = "SensorPuerta1";
const char* TIME_SERVER = "pool.ntp.org";
int myTimeZone = ARG; // change this to your time zone (see in timezone.h)


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

// Runs whenever the reedswitch changes state
ICACHE_RAM_ATTR void changeDoorStatus() {
  Serial.println("State changed");
  changeState = true;
}

void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  
} //end callback

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    
    // deviceId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has deviceId,username and password
    //please change following line to    if (client.connect(deviceId,userName,passWord))
    if (client.connect(deviceId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe(subscribetopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} //end reconnect()


void mqtt_msj(String code,String descript){
  time_t now;
  struct tm timeinfo;
  time(&now);
  String vProtocol= "100";
  StaticJsonDocument<300> doc;
  JsonObject JSONencoder = doc.to<JsonObject>();

  deviceLog+=1;

  JSONencoder["I"] = deviceId;
  JSONencoder["L"] = deviceLog;
  JSONencoder["T"] = now;
  JSONencoder["C"] = code;
  JSONencoder["D"] = descript;
  JSONencoder["V"] = vProtocol;
  
  char buffer[300];
  serializeJson(doc, buffer);
  size_t n = serializeJson(doc, buffer);
  client.publish(topic, buffer, n);
  
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  configTime(myTimeZone, 0, TIME_SERVER);      // get UTC time via NTP
  
  // Read the current door state
  pinMode(reedSwitch, INPUT_PULLUP);
  state = digitalRead(reedSwitch);

  // Set LED state to match door state
  pinMode(led, OUTPUT);
  digitalWrite(led, state);
  digitalWrite(BUILTIN_LED, state);
  
  
  // Set the reedswitch pin as interrupt, assign interrupt function and set CHANGE mode
  attachInterrupt(digitalPinToInterrupt(reedSwitch), changeDoorStatus, CHANGE);

  setup_wifi();
  client.setServer(mqtt_server_domain, mqtt_server_port);
  client.setCallback(callback);
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (changeState){
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval_sensor) {
      previousMillis = currentMillis;
      // If a state has occured, invert the current door state   
        state = !state;
        if(state) {
          doorState = "closed";
          mqtt_msj("R301",doorState);
          
        }
        else{
          doorState = "open";
          mqtt_msj("E301",doorState);
          
        }
        
        digitalWrite(led, state);
        digitalWrite(BUILTIN_LED, state);
        changeState = false;
        Serial.println(state);
        Serial.println(doorState);
      
    }  
  }
    //HearBEAT 
    if (millis() - lastMillis >= interval_hb) {
            lastMillis = millis();
            //hbLoop();
            mqtt_msj("E602","TEST");
        }
}
