#include <FS.h> 
#ifdef ESP32
  #include <SPIFFS.h>
#endif
#include <WiFiManager.h> 
#include <DNSServer.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "timezone.h"
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219_A(0x44);  //dirección 0x44
Adafruit_INA219 ina219_B;        // dirección(0x40)
TaskHandle_t Tarea_1;
DHTesp dht;
//------------------------------------------------------------------------------------------------------------------------------------------------------------
// ENTRADAS DEL MICRO:
#define SENS_OUT1 34   //Mide tensíon de salida 1
#define SENS_OUT2  35 //MIde tensión de salida 2
#define ALARM_OUT1 33  //Avisa cuando la salida 1 se corta por sobrecorriente
#define ALARM_OUT2 26  //Avisa cuando la salida 2 se corta por sobrecorriente
#define PIN_TEMP 14        //Sensor de temperatura y humedad
//------------------------------------------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// SALIDAS DEL MICRO:
#define ONOFF_OUT1 27     // Emite un pulso que enciende o apaga la salida 1
#define ONOFF_OUT2 25     // Emite un pulso que enciende o apaga la salida 2
#define PWM 13            // Controla la carga (y descarga) de batería por PWM
#define RESET_MODEM 12    // Emite un pulso que resetea la alimentación del Módem
#define CORTE_INPANEL 32  // En estado alto enciende Mosfet de entrada de panel. Se apaga cuando no se genera energía para aislar entrada.
#define LED_ESTADO 4      // (AZUL) Muestra estado de conexión
#define LED_BAT 15        // (VERDE) Muestra estado de carga y descarga de la batería
//------------------------------------------------------------------------------------------------------------------------------------------------------------
//VARIABLES Y CONSTANTES GENERALES:
#define CTE_SHUNT 4 // Cantidad de R de Shunt 0.1 Ohm en paralelo
#define DIVISOR  0.164     // Coeficiente del divisor de tensíon = R2/(R1+R2)=47k/(240k+47K)
#define SENSIB_1 0.00088   // Volt/muestra del ADC
#define SENSIB_2 0.00099   // Volt/muestra del ADC (reajuste para medir tensión de 5V)
#define V_FLOTE 13.8  
#define I_FLOTE 10   // mA
float I_load = 0;  // Corriente de carga total (Consumo interno más salidas): I_load = I_gen + I_bat
float I_bat = 0;   // (I_bat>0 : DESCARGANDO)  (I_bat<0: CARGANDO)
float Iabs_bat = 0;
float I_gen = 0;  // Corriente generada en la salida del convertidor DC/DC
float V_bat = 0;
float Vbat_min = 11.6; //Tensión de corte de las salidas
float V_reinicio = 13; //Tension de reinicio automatico de salidas
float VCC = 0;   //Tensión de alimentación principal (batería más generada)
float V_out1 = 0;   //Tensión de salida 1
float V_out2 = 0;   //Tensión de salida 2
float P_load = 0;  // Potencia total consumida por la carga: P_load = I_load*VCC
float P_gen = 0;
float temperatura;
float humedad;
byte Estado_bat = 0;  // 0 = Desconectada; 1 = Cargando; 2 = Descargando; 3 = Cargada
unsigned long prevMillis_High = 0;
unsigned long prevMillis_Low = 0;
boolean ledState1 = false; // led verde batería
unsigned long prevMillis_blink2 = 0;
boolean ledState2 = false; //led azul
boolean outState1 = false;  // Estado de salida 1
boolean outState2 = false; // Estado de salida 2
boolean outState3 = false; // Estado salida reset módem
boolean corte_bat = false; //Bandera en alto cuando se cortan salidas por bat. baja
boolean mem_out1 = false;  //memoria estado de salida cuando se produce el corte por bat. baja
boolean mem_out2 = false;
//------------------------------------------------------------------------------------------------------------------------------------------------------------
//Variables y Constantes de PWM:
int valorPWM = 200;
#define CANAL 0
#define PREC 9       // Precisión de 9 bits implica valores entre 0 y 511
#define FREC 5000    // Frecuencia en HZ
#define I_MAX 2000   //Corriente máxima de carga [mA]
#define PWM_MAX 511  // 2^PREC - 1
//------------------------------------------------------------------------------------------------------------------------------------------------------------
//VARIABLES Y CONSTANTES DE CONEXIÓN Y REPORTE:
/*const char* ssid = "ufi_26220"; 
const char* password = "1234567890";
const char* mqtt_server_domain = "testmqtt.iotcloud.studio"; 
const long mqtt_server_port = 51883;*/

/*const char* ssid = "IoT"; 
const char* password = "IoTcloud2019";
const char* mqtt_server_domain = "192.168.170.84"; //local 
const long mqtt_server_port = 1883;//local */

/*const char* ssid = "Fibertel WiFi288 2.4GHz";
const char* password = "0143512984";*/
const char* mqtt_server_domain = "testmqtt.iotcloud.studio";  // Remoto
const long mqtt_server_port = 51883;                          // Remoto

const char* mqttUser = "user";
const char* mqttPassword = "user";
const char* subscribetopic = "TEST/UPS";
const char* topic = "TEST/UPS";
String deviceId = "20000001";
String device_name = "IoT_Cloud_";
String ap_pass = "";
char mqtt_server[40] = "testmqtt.iotcloud.studio";;
char mqtt_port[6] = "51883";
char api_token[34] = "";
//default custom static IP
char static_ip[16] = "192.168.0.123";
char static_gw[16] = "192.168.0.1";
char static_sn[16] = "255.255.255.0";
char static_dns[16] = "8.8.8.8";
IPAddress ipAddr;

int deviceLog = 0;
String deviceDesciption = "PruebaUPS";
const char* TIME_SERVER = "8.8.8.8";
int myTimeZone = ARG;  // change this to your time zone (see in timezone.h)
char code[32] = "";
unsigned long previousMillis = 0;
unsigned long lastMillis = 0;
const long interval_sensor = 3600;
WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // deviceId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has deviceId,username and password
    //please change following line to    if (client.connect(deviceId,userName,passWord))
    if (client.connect(deviceId.c_str())) {
      Serial.println("connected");
      //once connected to MQTT broker, subscribe command if any
      client.subscribe(subscribetopic);
    } else {
      Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}  //end reconnect()
//------------------------------------------------------------------------------------------------------------------------------------------------------------
void mqtt_msj(String code, String descript) {
  time_t now;
  struct tm timeinfo;
  time(&now);
  String vProtocol = "100";
  StaticJsonDocument<300> doc;
  JsonObject JSONencoder = doc.to<JsonObject>();

  deviceLog += 1;

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
void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("entro callback") ;
  StaticJsonDocument<300> doc;
  deserializeJson(doc, payload, length);
  strlcpy(code, doc["C"] | "default", sizeof(code));
  if (strcmp(code, "E801") & !outState1) {
   Pulse(ONOFF_OUT1);
  }
  if (strcmp(code, "R801") & outState1) {
   Pulse(ONOFF_OUT1);
  } 
  if (strcmp(code, "E802") & !outState2) {
   Pulse(ONOFF_OUT2);
  }
  if (strcmp(code, "R802") & outState2) {
   Pulse(ONOFF_OUT2);
  } 
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//Mide la tensión en el PIN indicado
float mideTension(byte PIN){
   int RawValue =0;
   long AcumValue=0;
   int smooth_val =0;
   for(int i=0;i<64;i++){
      RawValue = analogRead(PIN);
       AcumValue+= RawValue;
   }
   smooth_val =(AcumValue/ 64);
   if (smooth_val>4095){smooth_val=4095;}
   float tension =0;
   if (smooth_val>1365) {tension = smooth_val* SENSIB_1 / DIVISOR ;}
   else{tension = smooth_val* SENSIB_2 / DIVISOR ;}
   return tension;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------
//Funcion de medición de corriente de batería:
float mideCorrienteBAT() {
  int RawValue = 0;
  long AcumValue = 0;
  float corriente = 0;
  for (int i = 0; i < 50; i++) {
    RawValue = ina219_B.getCurrent_mA();
    AcumValue = AcumValue + RawValue;
    delay(1);
  }
  corriente = (AcumValue / 50)*CTE_SHUNT;
  return corriente;
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//Genera un Pulso en el Pin indicado:
void Pulse(int PIN) {
digitalWrite(PIN, HIGH);
delay(200);
digitalWrite(PIN, LOW);
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//Funcion parpadeo led carga batería (verde):
void blink1(int interval_off) {
  unsigned long Millis= millis();
  if((Millis-prevMillis_High >= 100) & ledState1){
    ledState1=false;
    digitalWrite(LED_BAT, LOW);
    prevMillis_Low = Millis;
  }
   if((Millis-prevMillis_Low >= interval_off) & !ledState1){
    ledState1=true;
    digitalWrite(LED_BAT, HIGH);
    prevMillis_High = Millis;
  }
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//Funcion parpadeo led de estado (azul):
void blink2() {
  unsigned long Millis= millis();
  if(Millis-prevMillis_blink2 >= 500){
    prevMillis_blink2 = Millis;
  if (ledState2==LOW){ledState2= HIGH;}
  else{ledState2= LOW;}
  digitalWrite(LED_ESTADO,ledState2);
  }
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//Controla el PWM en funcion de la corriente de carga:
void ajustaPWMcargador() {
  float I_dif = Iabs_bat - I_MAX;
  float Iabs_dif = abs(I_dif);
  if (!I_dif == 0) {
    if ((Iabs_dif > 10) & (Iabs_bat > I_MAX)) {
      if (valorPWM >= 20) {
        valorPWM = valorPWM - 20;
        ledcWrite(CANAL, valorPWM);
        //Serial.print("PWM: ");
        //Serial.println(valorPWM);
      }
    }
    if ((Iabs_dif > 10) & (Iabs_bat < I_MAX)) {
      if (valorPWM <= (PWM_MAX - 20)) {
        valorPWM = valorPWM + 20;
        ledcWrite(CANAL, valorPWM);
        //Serial.print("PWM: ");
       // Serial.println(valorPWM);
      }
    }
    if ((Iabs_dif <= 10) & (Iabs_bat > I_MAX)) {
      if (valorPWM > 0) {
        valorPWM--;
        delay(1);
        ledcWrite(CANAL, valorPWM);
        //Serial.print("PWM: ");
        //Serial.println(valorPWM);
      }
    }
    if ((Iabs_dif <= 10) & (Iabs_bat < I_MAX)) {
      if (valorPWM < PWM_MAX) {
        valorPWM++;
        delay(1);
        ledcWrite(CANAL, valorPWM);
       // Serial.print("PWM: ");
       // Serial.println(valorPWM);
      }
    }
  }
}

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

String invertirCadena(String s) {
  String temporal = "";
  for (int m = s.length() - 1; m >= 0; m--)
    temporal += s[m];
  return temporal;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------
//SETUP:
void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore (loop_Tarea1,"Tarea1",10000,NULL,1,&Tarea_1,0);

   //-------------------------------------------------------------------------------------------------------------------------------------------------------------
 device_name = device_name + deviceId;
  const char* device_name_char=device_name.c_str();
  ap_pass = invertirCadena(deviceId);
  const char* ap_pass_char=ap_pass.c_str();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          //strcpy(api_token, json["api_token"]);

          if (json["ip"]) {
            Serial.println("setting custom ip from config");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            //strcpy(static_dns, json["DNS1"]);
            Serial.println(static_ip);
          } else {
            Serial.println("no custom ip in config");
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  Serial.println(static_ip);
  //Serial.println(api_token);
  Serial.println(mqtt_server);


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  //WiFiManagerParameter custom_text("<p>This is just a text paragraph</p>");
  
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  //WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 34);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setShowStaticFields(true);  // show Static IP  user Field 
 //wifiManager.setShowDnsFields(true); // show DNS  user Field
  //set static ip
  IPAddress _ip, _gw, _sn,_dns;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  _dns.fromString(static_dns);

  //wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn,_dns);

  //add all your parameters here
  //wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  //wifiManager.addParameter(&custom_api_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality(30);

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);
  wifiManager.setConfigPortalTimeout(120);
  
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(device_name_char, ap_pass_char)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
   wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn,_dns);
        if (WiFi.hostByName(mqtt_server, ipAddr) != 1)
        Serial.printf("ERROR: Unable to resolve host name- %s\n",mqtt_server);
    else
        Serial.printf("INFO: Host name resolved successfuly - %s\n", ipAddr.toString().c_str());

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  //strcpy(api_token, custom_api_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    //json["api_token"] = api_token;

    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();
    //json["dns"] = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }
  WiFi.mode(WIFI_STA);
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.dnsIP());
  //-------------------------------------------------------------------------------------------------------------------------------------------------------------
  
  configTime(myTimeZone, 0, TIME_SERVER);  // get UTC time via NTP
 
  client.setServer(mqtt_server, 51883);
  client.setCallback(callback);
  
  delay(500);
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//LOOP EN NÚCLEO 0:
void loop_Tarea1 (void*pvParameters) {
ledcSetup(CANAL, FREC, PREC);  //configuración PWM
  ledcAttachPin(PWM, CANAL);
  ledcWrite(CANAL, valorPWM);
  pinMode(ONOFF_OUT1, OUTPUT);
  digitalWrite(ONOFF_OUT1, LOW);
  pinMode(ONOFF_OUT2, OUTPUT);
  digitalWrite(ONOFF_OUT2, LOW);
  pinMode(CORTE_INPANEL, OUTPUT);
  digitalWrite(CORTE_INPANEL, LOW);
  pinMode(RESET_MODEM, OUTPUT);
  digitalWrite(RESET_MODEM, LOW);
  pinMode(LED_ESTADO, OUTPUT);
  digitalWrite(LED_ESTADO, LOW);
  pinMode(LED_BAT, OUTPUT);
  digitalWrite(LED_BAT, LOW);
 if (!ina219_A.begin()) {
    //Serial.println("Failed to find INA219_A chip");
    while (1) { delay(10); }
  }
  if (!ina219_B.begin()) {
    //Serial.println("Failed to find INA219_B chip");
    while (1) { delay(10); }
  }
  dht.setup(PIN_TEMP,DHTesp::DHT11);
  delay(500);
for(;;){
   /*if (client.connected()) {
   Serial.print("conectado ");;
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();*/
  //Serial.print("Tarea 1 se corre en el núcleo: ");
  //Serial.println(String (xPortGetCoreID()));
   I_gen = CTE_SHUNT *ina219_A.getCurrent_mA();
  if (I_gen > 10) {digitalWrite(CORTE_INPANEL, HIGH);}  // simulo diodo ideal con MOSFET
  else{digitalWrite(CORTE_INPANEL, LOW);}
  //VCC = ina219_A.getBusVoltage_V();
  //P_gen =  ina219_A.getPower_mW();
  
  float shuntvoltage_B = ina219_B.getShuntVoltage_mV();
  float busvoltage_B = ina219_B.getBusVoltage_V();
  V_bat = busvoltage_B + (shuntvoltage_B / 1000);

  I_bat = mideCorrienteBAT();
  Iabs_bat = abs(I_bat);
  if (Iabs_bat < 1) { Estado_bat = 0; digitalWrite(LED_BAT, LOW);} // Batería desconectada
  else {
    if (I_bat < 0) { 
      if ((V_bat>=V_FLOTE)& Iabs_bat<= I_FLOTE) {Estado_bat = 3; digitalWrite(LED_BAT, HIGH);} //Batería cargada
      else{Estado_bat = 1; ajustaPWMcargador(); blink1(100);} // Batería cargando
      }  
    if (I_bat > 0) { Estado_bat = 2; ledcWrite(CANAL, 511);blink1(2000);}   // Batería descargando
  }
  
  //I_load = I_gen + I_bat;
  //P_load = I_load * VCC;
  
  V_out1 = mideTension (SENS_OUT1);
  //Serial.println("VOUT1 = " V_out1);
  if (V_out1 >= 4){outState1 = true;}
  else{outState1 = false;}
  V_out2 = mideTension (SENS_OUT2);
 // Serial.println("VOUT2 = " V_out2 );
   if (V_out2 >= 4){outState2 = true;}
  else{outState2 = false;}
  if ((!corte_bat)&(V_bat <Vbat_min) & (Estado_bat == 2)) { //Corte de salidas por batería baja
    corte_bat = true;
    mem_out1 = outState1;
    mem_out2 = outState2;
    if(outState1){Pulse(ONOFF_OUT1);}
    if(outState2){Pulse(ONOFF_OUT2);}
     }
  if (corte_bat & V_bat>=V_reinicio){   //Reinicio de salidas con memoria
    corte_bat = false;
    outState1 = mem_out1;
    outState2 = mem_out2;
    if(outState1){Pulse(ONOFF_OUT1);}
    if(outState2){Pulse(ONOFF_OUT2);}
  }
   if (!client.connected()) {
    blink2();                        //parpadeo led azul
  }else {digitalWrite(LED_ESTADO,HIGH);}  
  TempAndHumidity data = dht.getTempAndHumidity();
  temperatura = data.temperature;
  humedad = data.humidity;
 // Serial.println("Temperatura: " + String(data.temperature, 2)+ "C");
 // Serial.println("Humedad: " + String(data.humidity, 1)+ "%");
 /* Serial.print("Tension VCC: "); Serial.print(VCC); Serial.println(" V");
  Serial.print("Tension de bateria: "); Serial.print(V_bat); Serial.println(" V");  
  Serial.print("Corriente generada: "); Serial.print(I_gen); Serial.println(" mA");
  Serial.print("Corriente de bateria: "); Serial.print(I_bat); Serial.println(" mA");
  Serial.print("Corriente total de carga : "); Serial.print(I_load); Serial.println(" mA");
  Serial.print("Potencia generada: "); Serial.print(P_gen); Serial.println(" mW");
  Serial.print("Potencia total de carga: "); Serial.print(P_load); Serial.println(" mW");
  */ delay(10);
  
}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//LOOP EN NÚCLEO 1:
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //Serial.print("paso2 ");
  unsigned long currentMillis = millis();
 if (currentMillis - previousMillis >= interval_sensor) {
    previousMillis = currentMillis;
   if (Estado_bat == 1) {
      mqtt_msj("Carg(mA)", dtostrf(Iabs_bat, 4, 2, msg));
      
    }
    if (Estado_bat == 2) {
      mqtt_msj("Desc(mA)", dtostrf(Iabs_bat, 4, 2, msg));
      
    }
    if (Estado_bat == 0) { mqtt_msj("NObat", dtostrf(Iabs_bat, 4, 2, msg)); }
    if (Estado_bat == 3) { mqtt_msj("Flote", dtostrf(Iabs_bat, 4, 2, msg)); }

    //mqtt_msj("VCC (V): ", dtostrf(VCC, 4, 2, msg));
    mqtt_msj("Vbat (V)", dtostrf(V_bat, 4, 2, msg));
    mqtt_msj("Igen(mA)", dtostrf(I_gen, 4, 2, msg));
    mqtt_msj("Iload(mA)", dtostrf(I_load, 4, 2, msg));
    mqtt_msj("Temp", dtostrf(temperatura, 4, 2, msg));
    mqtt_msj("Hum(%)", dtostrf(humedad, 4, 2, msg));
    mqtt_msj("Vout1(V)", dtostrf(V_out1, 4, 2, msg));
    //mqtt_msj("P_gen (mW): ", dtostrf(P_gen, 4, 2, msg));
    //mqtt_msj("P_load(mW): ", dtostrf(P_load, 4, 2, msg));
    //Serial.println(" ");
  }
}