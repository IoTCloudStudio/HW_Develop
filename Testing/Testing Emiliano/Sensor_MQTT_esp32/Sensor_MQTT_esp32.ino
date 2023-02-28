#include <MQTTPubSubClient.h>



#include <WiFi.h>
//#include <PubSubClient.h>

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
const long interval = 1500;

const char* ssid = "Fibertel WiFi288 2.4GHz";
const char* password = "0143512984";
const char* mqtt_server_domain = "testmqtt.iotcloud.studio"; // Remoto: "testmqtt.iotcloud.studio";
const long mqtt_server_port = 51883;// Remoto: 51883;
const char *prefixtopic = "ALARM/";                            // We'll use the prefix to describe a 'family' of devices.
const char *subscribetopic[] = {"PUERTA1"};     // Topics that we will subscribe to within that family.
const char* topic = "PUERTA1";     // Topics that we will subscribe to within that family.

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
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe("OsoyooCommand");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} //end reconnect()

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  
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
    if(currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      // If a state has occured, invert the current door state   
        state = !state;
        if(state) {
          doorState = "closed";
          
        }
        else{
          doorState = "open";
          
        }
        char message[58];
        doorState.toCharArray(message,58);
        client.publish(topic, message);
        digitalWrite(led, state);
        digitalWrite(BUILTIN_LED, state);
        changeState = false;
        Serial.println(state);
        Serial.println(doorState);
      
    }  
  }
}
