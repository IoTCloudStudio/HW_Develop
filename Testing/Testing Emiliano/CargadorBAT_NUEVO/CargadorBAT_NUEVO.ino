#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "timezone.h"
#include <ArduinoJson.h>

const char* ssid = "ufi_26220"; 
const char* password = "1234567890";
const char* mqtt_server_domain = "testmqtt.iotcloud.studio"; //local 
const long mqtt_server_port = 51883;//local

//const char* ssid = "IoT"; 
//const char* password = "IoTcloud2019";
//const char* mqtt_server_domain = "192.168.170.84"; //local 
//const long mqtt_server_port = 1883;//local

//const char* ssid = "Fibertel WiFi288 2.4GHz";
//const char* password = "0143512984";
//const char* mqtt_server_domain = "testmqtt.iotcloud.studio"; // Remoto
//const long mqtt_server_port = 51883; // Remoto

const char* mqttUser = "user";
const char* mqttPassword = "user";                           // We'll use the prefix to describe a 'family' of devices.
const char* subscribetopic = "TEST/UPS";     // Topics that we will subscribe to within that family.
const char* topic = "TEST/UPS";     // Topics that we will subscribe to within that family.
String deviceId = "666";
int deviceLog = 0;
String deviceDesciption = "PruebaUPS";
const char* TIME_SERVER = "pool.ntp.org";
int myTimeZone = ARG; // change this to your time zone (see in timezone.h)
char code[32] = "";
unsigned long previousMillis = 0;
unsigned long lastMillis = 0; 
const long interval_sensor = 3000;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

// ENTRADAS:
#define VPAN 36         // Mide tensión en terminal negativo del panel
#define VBAT 39         
#define IBAT 35             // Mide corriente de carga y descarga de la batería
#define ICARGA 34
#define IPANEL 32
// SALIDAS:
#define CORTE_SALIDA 23

//Constantes del ADC:
#define MUESTRAS 100                   //Muestras para hacer promedio de mediciones 
#define VoltPorMuestra 0.000887      // Sale de hacer 3.3 V/4095 muestras

//Variables y Constantes de Medicion de Corriente:
//#define I_MAX 1               //Corriente máxima de carga = 1 amper
#define divisor_3 0.82 
#define divisor_4 0.6397
#define divisor_5 0.6397    // Coeficiente del divisor de tensíon = R2/(R1+R2)
#define V0_hall 2.475
#define sensib_1 0.17        // Volt/Amper
#define sensib_2 0.100
#define sensib_3 0.100
float I_CARGA;
float I_BAT;                  // I_BAT>0 en CARGA y I_BAT<0 en DESCARGA
float I_PANEL;

//Variables y Constantes de Medicion de Tension:
#define V_MAX 14.4             //Máxima tensión de carga de la batería
#define V_CORTE 11             //Tensión a la que se corta la salida para protección por sobredescarga  
#define divisor_1  0.183   // Coeficiente del divisor de tensíon = R2/(R1+R2)
#define divisor_2  0.185
float tensionBAT = 0;         
float tensionPANEL = 0;  
float V_PANEL = 0;   
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

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
//-------------------------------------------------------------------------------------------------------------------------------------------

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
//------------------------------------------------------------------------------------------------------------------------------------------------------------

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
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void callback(char* topic, byte* payload, unsigned int length){
   
  StaticJsonDocument<300> doc;

  deserializeJson(doc, payload, length);

  strlcpy(code, doc["C"] | "default", sizeof(code));
 

  if (strcmp (code, "E801") == 0){
    digitalWrite (CORTE_SALIDA,LOW);
    //Serial.println("E801");
  }
  else if (strcmp (code, "R801") == 0){
    digitalWrite (CORTE_SALIDA,HIGH);
    //Serial.println("R801");
  }

}
//---------------------------------------------------------------------------------------------------------------------------------------------------------

//Funcion de medición de corriente con sensor efecto Hall:
float mideCorriente(byte PIN, float divisor,float sensib){
   int RawValue = 0;
   long AcumValue= 0;
   float corriente = 0;
   float VoltHall = 0;   
   for(int i=0;i<MUESTRAS;i++){
      RawValue = analogRead(PIN);
      AcumValue= AcumValue + RawValue;
      delay(1);
   }
   VoltHall = (((AcumValue / MUESTRAS)* VoltPorMuestra) / divisor) - V0_hall;
   corriente = VoltHall/sensib;
   return corriente;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------

//Funcion de medicion de Tensión. Mide la tensión en el PIN indicado y lo convierte por el divisor de tensión.
float mideTension(byte PIN, float divisor){
   int RawValue =0;
   long AcumValue=0;
   for(int i=0;i<MUESTRAS;i++){
      RawValue = analogRead(PIN);
       AcumValue= AcumValue + RawValue;
      delay(1);
   }
   float tension = (AcumValue/ MUESTRAS)* VoltPorMuestra / divisor ;
   return tension;
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------

//SETUP:
void setup(){
   Serial.begin(115200);
   configTime(myTimeZone, 0, TIME_SERVER);      // get UTC time via NTP
   setup_wifi();
   client.setServer(mqtt_server_domain, mqtt_server_port);
   client.setCallback(callback);
   pinMode(CORTE_SALIDA, OUTPUT);
   delay(500);
  
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

void loop(){
    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval_sensor) {
      previousMillis = currentMillis;
   I_CARGA = mideCorriente(ICARGA, divisor_3, sensib_1);   
   I_BAT = mideCorriente(IBAT, divisor_4, sensib_2);
   I_PANEL = mideCorriente(IPANEL, divisor_5, sensib_3);
   mqtt_msj("I_CARGA",dtostrf(I_CARGA, 4, 6, msg));
   Serial.print("I_CARGA: ");
   Serial.println(I_CARGA,3);
   mqtt_msj("I_BAT",dtostrf(I_BAT, 4, 6, msg));
   Serial.print("I_BAT: ");
   Serial.println(I_BAT,3);
   mqtt_msj("I_PANEL",dtostrf(I_PANEL, 4, 6, msg));
   Serial.print("I_PANEL: ");
   Serial.println(I_PANEL,3);
   tensionBAT = mideTension(VBAT, divisor_2); 
    mqtt_msj("VBAT",dtostrf(tensionBAT, 4, 6, msg));
   Serial.print("VBAT: ");
   Serial.println(tensionBAT,3);
   tensionPANEL = mideTension(VPAN, divisor_1); 
   V_PANEL = tensionBAT - tensionPANEL;
    mqtt_msj("VPANEL",dtostrf(V_PANEL, 4, 6, msg));
   Serial.print("VPANEL: ");
   Serial.println(V_PANEL,3);
Serial.println(" ");
Serial.println(" ");
    }
}

