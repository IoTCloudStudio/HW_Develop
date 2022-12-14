#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "timezone.h"
#include <ArduinoJson.h>

//mosquitto_pub -h 192.168.170.84 -t ALARM/PUERTA3 -m "{\"I\":3,\"L\":22,\"T\":1668137262,\"C\":\"E801\",\"D\":\"closed\",\"V\":\"100\"}"
// Set GPIOs for LED and reedswitch
const int reedSwitch = 5;
const int led = 2; //optional
const byte TONE_PIN = 12;
const int ALARM_BEEP_1 = 4186;
const int ALARM_BEEP_2 = 4699;

const int ALARM_TONE_LENGTH = 200;
const int ALARM_TONE_PAUSE = 800;
const int ALARM_TONE_REPEAT = 6;
 
// Detects whenever the door changed state
bool changeState = false;

// Holds reedswitch state (1=opened, 0=close)
bool state;
bool silenciar=0;
String doorState;


// Auxiliary variables (it will only detect changes that are 1500 milliseconds apart)
unsigned long previousMillis = 0;
unsigned long lastMillis = 0; 
const long interval_sensor = 1500;
const long interval_hb = 60000;

//const char* ssid = "CMR";
//const char* password = "Rosario2020";
//const char* mqtt_server_domain = "192.168.2.93";
//const long mqtt_server_port = 1883;
const char* ssid = "IoT";
const char* password = "IoTcloud2019";
const char* mqtt_server_domain = "192.168.170.84"; // Remoto: "testmqtt.iotcloud.studio";//
const long mqtt_server_port = 1883;// Remoto: 51883;
const char* mqttUser = "user";
const char* mqttPassword = "user";                           // We'll use the prefix to describe a 'family' of devices.
const char* subscribetopic = "ALARM/PUERTA3";     // Topics that we will subscribe to within that family.
const char* topic = "ALARM/PUERTA3";     // Topics that we will subscribe to within that family.
String deviceId = "3";
int deviceLog = 0;
String deviceDesciption = "SensorPuerta3";
const char* TIME_SERVER = "pool.ntp.org";
int myTimeZone = ARG; // change this to your time zone (see in timezone.h)
char code[32] = "";


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

// Runs whenever the reedswitch changes state
ICACHE_RAM_ATTR void changeDoorStatus() {
  Serial.println("State changed");
  changeState = true;
}

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
    //tone(TONE_PIN, (count % 2) ? ALARM_BEEP_1 : ALARM_BEEP_2, ALARM_TONE_LENGTH);
    digitalWrite(TONE_PIN, HIGH);
  }
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

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<300> doc;
  
  
    deserializeJson(doc, payload, length);
    strlcpy(code, doc["C"] | "default", sizeof(code));
    if (strcmp (code,"E801") == 0){
      silenciar = 1;
      Serial.println("Silenciado");
      Serial.println("-----------------------");
      Serial.println();
      
    }
    else if (strcmp (code,"E802") == 0){
      silenciar = 0;
      Serial.println("Sin silenciar");
      Serial.println("-----------------------");
      Serial.println();
      
    }
  

}

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
  pinMode(TONE_PIN, OUTPUT);
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
      state = digitalRead(reedSwitch);
      // If a state has occured, invert the current door state   
        state = !state;
        if(state) {
          doorState = "closed";
          mqtt_msj("R301",doorState);
          //noTone(14);
          digitalWrite(TONE_PIN, LOW);
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
  unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval_sensor*4) {
      previousMillis = currentMillis;
      state = digitalRead(reedSwitch);
      // If a state has occured, invert the current door state   
        state = !state;
        if(state) {
          doorState = "closed";
          mqtt_msj("R301",doorState);
          //noTone(14);
          digitalWrite(TONE_PIN, LOW);
          
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
     if (doorState == "open" && silenciar==0) {
    alarmSound();
  }
    //HearBEAT 
    if (millis() - lastMillis >= interval_hb) {
            lastMillis = millis();
            //hbLoop();
            mqtt_msj("E602","TEST");
        }
}
