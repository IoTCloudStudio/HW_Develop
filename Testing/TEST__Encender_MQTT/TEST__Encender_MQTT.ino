/*
 * File: mqtt-LED.ino
 * 
 * By: Andrew Tuline
 * 
 * Date: June, 2019
 * 
 * This program demonstrates using MQTT messaging with robust network re-connection to blink the internal LED of an ESP8266 based WeMOS D1 Mini.
 * 
 * The Android client is an application called 'IoT MQTT Panel'. There's a Pro version which has a few more features and no advertising.
 * 
 * 
 * Tested working configurations include:
 * 
 * 1) ESP8266 running PubSubclient DHCP'ed to my home network, cloudMQTT.com as the broker and IoT MQTT Panel as the publisher/client on my Android.
 * 2) ESP8266 running PubSubClient DHCP'ed to my home network, MQTT Broker App on my Android and IoT MQTT Panel as the publisher/client on my Android.
 * 3) ESP8266 running PubSubClient DHCP'ed to my home network, Mosquitto running on my Windows workstation and IoT MQTT Panel as the publisher/client on my Android. Only works when I'm at home.
 * 4) ESP8266 running PubSubClient DHCP'ed to my tethered Android (as 192.168.43.1), MQTT Broker App on my Android and IoT MQTT Panel as the publisher/client on my Android.
 * 
 * 
 * Information  Value(s)  Type
 * ===========  =======   ====  
 * MQTT Topic   LED/LED1  String (or char*)                       // The Subscribed Topic.
 *    Payload   0, 1      Byte, converted to string               // The Control Panel outputs ASCII strings as the payload.
 * 
 * 
 * To run this:
 * 
 * 1) Configure and enable tethering on your Android phone with the credentials shown below.
 * 2) Install configure and start an MQTT broker (i.e. MQTT Broker App) on your Android phone and configure with credentials shown below.
 * 3) Alternatively setup an MQTT broker on the Internet, i.e. cloudMQTT.com or on your workstation/laptop.
 * 4) Install an MQTT client (i.e. IoT MQTT Panel) on your Android phone and configure with the configuration and credentials shown below.
 * 5) Setup a device and panel to communicate between Control Panel <---> MQTT Broker <---> ESP8266
 *
 *
 * For more information, see: http://tuline.com/getting-started-with-esp8266-and-mqtt/
 * 
 */


#include <ESP8266WiFi.h>                                    // Should be included when you install the ESP8266.
#include <PubSubClient.h>                                   // https://github.com/knolleary/pubsubclient

const char* ssid = "IoT";                          // WiFi configuration for Android tethering network. The Android IP address is hard coded at 192.168.43.1.
const char* password = "IoTcloud2019";

// My Phone as a tethered device (working)                  // MQTT Broker application on Android credentials - using the Android tethered WiFi.
const char* mqttServer = "192.168.170.74";
const int mqttPort = 1883;
const char* mqttUser = "test";
const char* mqttPassword = "vivero3382";
const char* mqttID ="12";                                   // Must be UNIQUE for EVERY display!!!


const char *prefixtopic = "LED/";                            // We'll use the prefix to describe a 'family' of devices.
const char *subscribetopic[] = {"LED1", "LED2", "LED3"};     // Topics that we will subscribe to within that family.

char message_buff[100];
 
WiFiClient espClient;
PubSubClient client(espClient);



void setup() {
 
  Serial.begin(115200);

  pinMode(BUILTIN_LED, OUTPUT);

  client.setServer(mqttServer, mqttPort);                                                 // Initialize MQTT service. Just once is required.
  client.setCallback(callback);                                                           // Define the callback service to check with the server. Just once is required.
  
} // setup()


  
void loop() {

  networking();                                                                             // We need to run this continually in case we disconnect from the MQTT broker.

} // loop()



void callback(char *topic, byte *payload, unsigned int length) {
  
  Serial.print("Message arrived: ["); Serial.print(topic); Serial.println("]");             // Prints out anything that's arrived from broker and from the topic that we've subscribed to.

  int i; for (i = 0; i < length; i++) message_buff[i] = payload[i]; message_buff[i] = '\0';  // We copy payload to message_buff because we can't make a string out of payload.
  
  String msgString = String(message_buff);                                                  // Finally, converting our payload to a string so we can compare it.
//  uint8_t msgVal = msgString.toInt();                                                     // We can optionally convert that string to an integer.

  Serial.print("Message: ");  Serial.println(msgString);
  if (strcmp(topic, "LED/LED1") == 0) {                                                     // Returns 0 if the strings are equal, so we have received our topic.
    if (msgString == "1") {                                                                 // Payload of our topic is '1', so we'll turn on the LED.
      digitalWrite(BUILTIN_LED, LOW);                                                       // PIN HIGH will switch ON the relay
    }
    if (msgString == "0") {                                                                 // Payload of our topic is '0', so we'll turn off the LED.
      digitalWrite(BUILTIN_LED, HIGH);                                                      // PIN LOW will switch OFF the relay
    }
  } // if strcmp
  
} // callback()



void networking() {                                                                         // Asynchronous network connect routine with MQTT connect and re-connect. The trick is to make it re-connect when something breaks.

  static long lastReconnectAttempt = 0;
  static bool wifistart = false;                                                            // State variables for asynchronous wifi & mqtt connection.
  static bool wificonn = false;

  if (wifistart == false) {
    WiFi.begin(ssid, password);                                                             // Initialize WiFi on the ESP8266. We don't care about actual status.
    wifistart = true;
  }

  if (WiFi.status() == WL_CONNECTED && wificonn == false) {
    Serial.print("IP address:\t"); Serial.println(WiFi.localIP());                          // This should just print once.
    wificonn = true;    
  }

  if (WiFi.status() != WL_CONNECTED) wificonn = false;                                      // Toast the connection if we've lost it.

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {                               // Non-blocking re-connect to the broker. This was challenging.
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      if (reconnect()) {                                                                    // Attempt to reconnect.
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }

} // networking()



boolean reconnect() {                                                                       // Here is where we actually connect/re-connect to the MQTT broker and subscribe to our topics.
  
  if (client.connect(mqttID, mqttUser, mqttPassword )) {
    
    Serial.println("MQTT connected.");  
//    client.publish("LED1", "Hello from ESP8266!");                                        // Sends topic and payload to the MQTT broker.
    for (int i = 0; i < (sizeof(subscribetopic)/sizeof(int)); i++) {                        // Subscribes to list of topics from the MQTT broker. This whole loop is well above my pay grade.
      String mypref =  prefixtopic;                                                         // But first, we need to get our prefix.
      mypref.concat(subscribetopic[i]);                                                     // Concatenate prefix and the topic together with a little bit of pfm. 
      client.subscribe((char *) mypref.c_str());                                            // Now, let's subscribe to that concatenated and converted mess.
      Serial.println(mypref);                                                               // Let's print out each subscribed topic, just to be safe.
    }
  } // if client.connected()

  return client.connected();
  
} // reconnect()
