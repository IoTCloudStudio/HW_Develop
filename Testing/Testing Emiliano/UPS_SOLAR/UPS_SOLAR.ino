#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "timezone.h"
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219_A; // dirección(0x40)
Adafruit_INA219 ina219_B(0x44);

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// ENTRADAS DEL MICRO:

//------------------------------------------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// SALIDAS DEL MICRO:
#define ENABLE_OUT1 23  
//#define ENABLE_OUT2 19
#define PWM 18
//#define RESET_MODEM 17
//------------------------------------------------------------------------------------------------------------------------------------------------------------

//VARIABLES:
float I_load = 0;                  // Corriente de carga total: I_load = I_gen + I_bat
float I_bat = 0;                  // (I_bat>0 : DESCARGANDO)  (I_bat<0: CARGANDO)
float Iabs_bat = 0;
float I_gen = 0;                  // Corriente generada en la salida del convertidor DC/DC
float V_bat = 0;         
float V_out = 0;                  //Tensión de salida
float P_load = 0;                 // Potencia total consumida por la carga: P_load = I_load*V_out                  
float P_gen = 0;
byte Estado_bat = 0;             // 0 = Desconectada; 1 = Cargando; 2 = Descargando
//------------------------------------------------------------------------------------------------------------------------------------------------------------
//Variables y Constantes de PWM:
int valorPWM = 200;           
#define CANAL 0            
#define PREC 9                // Precisión de 9 bits implica valores entre 0 y 511
#define FREC 5000              // Frecuencia en HZ
#define I_MAX    1500       //Corriente máxima de carga =  mili Amper
#define PWM_MAX  511        // 2^PREC - 1
//------------------------------------------------------------------------------------------------------------------------------------------------------------
const char* ssid = "ufi_26220"; 
const char* password = "1234567890";
const char* mqtt_server_domain = "testmqtt.iotcloud.studio"; 
const long mqtt_server_port = 51883;

const char* mqttUser = "user";
const char* mqttPassword = "user";            
const char* subscribetopic = "TEST/UPS";     
const char* topic = "TEST/UPS";            
String deviceId = "666";
int deviceLog = 0;
String deviceDesciption = "PruebaUPS";
const char* TIME_SERVER = "pool.ntp.org";
int myTimeZone = ARG; // change this to your time zone (see in timezone.h)
char code[32] = "";
unsigned long previousMillis = 0;
unsigned long lastMillis = 0; 
const long interval_sensor = 300000;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
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
//------------------------------------------------------------------------------------------------------------------------------------------------------------

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
    digitalWrite (ENABLE_OUT1,LOW);
    //Serial.println("E801");
  }
  else if (strcmp (code, "R801") == 0){
    digitalWrite (ENABLE_OUT1,HIGH);
    //Serial.println("R801");
  }

}
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//Funcion de medición de corriente:
float mideCorrienteBAT(){
   int RawValue = 0;
   long AcumValue= 0;
   float corriente = 0;  
   for(int i=0;i<50;i++){
      RawValue = ina219_B.getCurrent_mA();;
      AcumValue= AcumValue + RawValue;
      delay(1);
   }
   corriente = (AcumValue / 50);
   return corriente;
}
//Controla el PWM en funcion de la corriente de carga:
void ajustaPWMcargador(){
   float I_dif = Iabs_bat - I_MAX;
   float Iabs_dif = abs(I_dif);
   if(!I_dif == 0){
   if((Iabs_dif > 10) & (Iabs_bat > I_MAX) ){  
     if(valorPWM>=20){
         valorPWM = valorPWM - 20;
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM);}
    } 
    if((Iabs_dif > 10) & (Iabs_bat < I_MAX) ){  
     if(valorPWM<=(PWM_MAX -20)){
         valorPWM = valorPWM + 20 ;
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM); }
    }
   if((Iabs_dif <= 10) & (Iabs_bat > I_MAX)){
         if(valorPWM>0){
         valorPWM-- ;
         delay(1);
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM);}
             } 
    if((Iabs_dif <= 10) & (Iabs_bat < I_MAX)){       
         if(valorPWM<PWM_MAX){
         valorPWM++;
         delay(1);
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM); }
      }
   }
   }
   
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//SETUP:
void setup(){
   Serial.begin(115200);
   configTime(myTimeZone, 0, TIME_SERVER);      // get UTC time via NTP
   setup_wifi();
   client.setServer(mqtt_server_domain, mqtt_server_port);
   client.setCallback(callback);
   ledcSetup(CANAL,FREC,PREC);
   ledcAttachPin(PWM,CANAL);
   ledcWrite(CANAL, valorPWM);
   pinMode(ENABLE_OUT1, OUTPUT); 
    if (! ina219_A.begin()) {
    Serial.println("Failed to find INA219_A chip");
    while (1) { delay(10); }
  }
  if (! ina219_B.begin()) {
    Serial.println("Failed to find INA219_B chip");
    while (1) { delay(10); }
  }
   delay(500);
  
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//LOOP:
void loop(){
    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval_sensor) {
      previousMillis = currentMillis;

  V_out= ina219_A.getBusVoltage_V();
  I_gen = ina219_A.getCurrent_mA();
  P_gen = ina219_A.getPower_mW();
  
  float shuntvoltage_B = ina219_B.getShuntVoltage_mV();
  float busvoltage_B = ina219_B.getBusVoltage_V();
  V_bat = busvoltage_B + (shuntvoltage_B / 1000);
  
  I_bat = mideCorrienteBAT();
  Iabs_bat = abs(I_bat);
  if (Iabs_bat < 10){Estado_bat = 0;}
  if (Iabs_bat >= 10){
    if (I_bat<0) {Estado_bat = 1;}
    if(I_bat>0) {Estado_bat = 2;}
  }
  I_load = I_gen + I_bat;
  P_load = I_load*V_out;

  ajustaPWMcargador();  
  
  /*Serial.print("Tension de salida: "); Serial.print(V_out); Serial.println(" V");
  Serial.print("Tension de bateria: "); Serial.print(V_bat); Serial.println(" V");  
  Serial.print("Corriente generada: "); Serial.print(I_gen); Serial.println(" mA");
  Serial.print("Corriente de bateria: "); Serial.print(I_bat); Serial.println(" mA");
  Serial.print("Corriente total de carga : "); Serial.print(I_load); Serial.println(" mA");
  Serial.print("Potencia generada: "); Serial.print(P_gen); Serial.println(" mW");
  Serial.print("Potencia total de carga: "); Serial.print(P_load); Serial.println(" mW");
   */
   if (Estado_bat == 1){mqtt_msj(" Bateria cargando: I_bat (mA) = ", dtostrf(Iabs_bat, 4, 2, msg));
                      mqtt_msj("V_bat (V): ", dtostrf(V_bat, 4, 2, msg));}   
   if (Estado_bat == 2){mqtt_msj("Bateria descargando: I_bat (mA) = ", dtostrf(Iabs_bat, 4, 2, msg));
                      mqtt_msj("V_bat (V): ",dtostrf(V_bat, 4, 2, msg));}  
   if (Estado_bat== 0){mqtt_msj("Bateria desconectada : I_bat (mA) = ", dtostrf(Iabs_bat, 4, 2, msg));}   
 
   mqtt_msj("V_out (V): ",dtostrf(V_out, 4, 2, msg));
   mqtt_msj("I_gen (mA): ",dtostrf(I_gen, 4, 2, msg));
   mqtt_msj("I_load (mA): ",dtostrf(I_load, 4, 2, msg));
   mqtt_msj("P_gen (mW): ",dtostrf(P_gen, 4, 2, msg));
   mqtt_msj("P_load(mW): ",dtostrf(P_load, 4, 2, msg));
   Serial.println(" ");
    }
}

