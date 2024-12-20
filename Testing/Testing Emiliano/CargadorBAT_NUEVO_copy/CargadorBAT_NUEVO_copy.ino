#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "timezone.h"
#include <ArduinoJson.h>

const char* ssid = "IoT"; //"Fibertel WiFi288 2.4GHz"
const char* password = "IoTcloud2019";//0143512984
const char* mqtt_server_domain = "192.168.170.84"; // Remoto: "testmqtt.iotcloud.studio"; local 192.168.170.84
const long mqtt_server_port = 1883;// Remoto: 51883; local 1883
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
#define VCC 36         // Mide tensión en terminal positivo de la batería 
#define VBAT 39         // Mide tensión en terminal negativo de la batería
#define IBAT 34             // Mide corriente de carga y descarga de la batería
// SALIDAS:
#define PWM 23
#define RELE_SAL 22
#define PULSADOR 21

//Constantes del ADC:
#define MUESTRAS 64                   //Muestras para hacer promedio de mediciones 
#define VoltPorMuestra 0.00089      // Sale de hacer 3.3 V/4095 muestras

//Variables y Constantes de Medicion de Corriente:
#define I_MAX 1               //Corriente máxima de carga = 1 amper
#define divisor_3 0.637      // Coeficiente del divisor de tensíon = R2/(R1+R2)
#define Coef_Carga 12.8       
#define Coef_Descarga 13.7
#define V0_hall 2.47
float I_BAT;                  // I_BAT>0 en CARGA y I_BAT<0 en DESCARGA

//Variables y Constantes de PWM:
int valorPWM = 255;            // Con PWM máximo el MOSFET arranca apagado (está invertido por transistor de acople)
#define CANAL 0            
#define PREC 8                 // Precisión de 9 bits implica valores entre 0 y 511
#define FREC 5000              // Frecuencia en HZ

//Variables y Constantes de Medicion de Tension:
#define V_MAX 14.4             //Máxima tensión de carga de la batería
#define V_CORTE 11             //Tensión a la que se corta la salida para protección por sobredescarga  
#define divisor_1  0.183   // Coeficiente del divisor de tensíon = R2/(R1+R2)
#define divisor_2  0.183
float tensionBAT = 0;         
float tensionVCC = 0;     


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

void callback(char* topic, byte* payload, unsigned int length){
   
  StaticJsonDocument<300> doc;

  deserializeJson(doc, payload, length);

  strlcpy(code, doc["C"] | "default", sizeof(code));
 

  if (strcmp (code, "E801") == 0){
    digitalWrite (RELE_SAL,LOW);
    Serial.println("E801");
  }
  else if (strcmp (code, "R801") == 0){
    digitalWrite (RELE_SAL,HIGH);
    Serial.println("R801");
  }

}

//Funcion Setup:
void setup(){
   Serial.begin(115200);
   configTime(myTimeZone, 0, TIME_SERVER);      // get UTC time via NTP
   ledcSetup(CANAL,FREC,PREC);
   ledcAttachPin(PWM,CANAL);
   valorPWM = 255;
   ledcWrite(CANAL, valorPWM);
   setup_wifi();
   client.setServer(mqtt_server_domain, mqtt_server_port);
   client.setCallback(callback);
   pinMode(RELE_SAL, OUTPUT);
   pinMode(PULSADOR, INPUT);
   delay(500);
  
}

//Funcion Loop:
void loop(){
    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval_sensor) {
      previousMillis = currentMillis;
   I_BAT = mideCorrienteBAT();
   mqtt_msj("I_BAT",dtostrf(I_BAT, 4, 6, msg));
   Serial.print("I_BAT: ");
   Serial.println(I_BAT,3);
   tensionVCC = mideTension(VCC, divisor_1); 
    mqtt_msj("VCC",dtostrf(tensionVCC, 4, 6, msg));
   Serial.print("VCC: ");
   Serial.println(tensionVCC,3);
   tensionBAT = mideTension(VBAT, divisor_2); 
    mqtt_msj("VBAT",dtostrf(tensionBAT, 4, 6, msg));
   Serial.print("VBAT: ");
   Serial.println(tensionBAT,3);
   ajustaPWMcargador();
//boolean p_state = digitalRead(PULSADOR);
//if (p_state==HIGH){
//digitalWrite (RELE_SAL,HIGH);
//}  
//if (p_state==LOW){
//digitalWrite (RELE_SAL,LOW);
//} 
Serial.println(" ");
Serial.println(" ");
    }
}

//Funcion de medición de corriente con sensor efecto Hall:
float mideCorrienteBAT(){
   int RawValue = 0;
   long AcumValue= 0;
   float corriente = 0;
   float VoltHall = 0;   
   for(int i=0;i<MUESTRAS;i++){
      RawValue = analogRead(IBAT);
      AcumValue= AcumValue + RawValue;
      delay(1);
   }
   VoltHall = (((AcumValue / MUESTRAS)* VoltPorMuestra) / divisor_3) - V0_hall;
   if (VoltHall > 0){      //Condición de que se está cargando
      corriente = VoltHall* Coef_Carga;
   }
  if (VoltHall<0){    //Condición de descarga 
    corriente = VoltHall* Coef_Descarga;
   }
   return corriente;
}

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

//Controla el PWM en funcion de la corriente de carga:
void ajustaPWMcargador(){
   if(I_BAT < I_MAX){
      if(tensionBAT < V_MAX){
         if(valorPWM>0)
            valorPWM--;
         delay(1);
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM); 
         delay (1000);         
      } 
      else{
         Serial.println("Límite de Tensión");        
         if(valorPWM<255)
            valorPWM++;
         delay(1);
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM); 
         delay(1000);
         
      }
   }
   else{
      Serial.println("Límite de corriente");
      if(valorPWM<511)
            valorPWM++;
         delay(1);
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM); 
         delay (1000);
   }
}   