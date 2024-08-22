
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

RCSwitch mySwitch = RCSwitch();
#define PIND0 0
#define PIND1 2
//#define PIND0 2
//#define PIND1 0
#define INDATA 3
long valor;
long bit;
String valorstring;
String truncstring;
int numero;
//#define LED_PIN 1
#define CANTBITCODE 24 // 26 BIT FORMATO WIEGAND MENOS 2 BIT DE PARIDAD
#define BITWIEGAND 26 // 26 BIT FORMATO WIEGAND
int binario[CANTBITCODE];
int bitparidadpar;
int bitparidadimpar;
int contpar = 0;
int contimpar = 0;
int datawiegand [BITWIEGAND];

const char* ssid = "Claro-WiFi-58DE";
const char* password = "ADQDT0TGLFJ";
const char* mqtt_server_domain = "testmqtt.iotcloud.studio";
const long mqtt_server_port = 51883;
const char* subscribetopic = "ALARM/#";
char code[32] = "";

WiFiClient espClient;
PubSubClient client(espClient);


void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("entro callback") ;
  StaticJsonDocument<300> doc;
  deserializeJson(doc, payload, length);
  strlcpy(code, doc["C"] | "default", sizeof(code));
  if (strcmp(code, "E801")==0) {
   envioWiegand(2925717);
  }
}

void envioWiegand(int numero){
//........AHORA LO PASO A FORMATO BINARIO Y GENERO BITS DE PARIDAD
   contpar = 0;
   contimpar = 0;
   bitparidadpar=0;
   bitparidadimpar=0;

    for(int i=0;i<CANTBITCODE;i++){
      binario [i]= bitRead(numero,CANTBITCODE-1-i);
      //....OBTENGO BITS DE PARIDAD
      if (i<CANTBITCODE/2){if(binario [i]==1){contpar=contpar+1;}}
      if(i>=CANTBITCODE/2){if(binario [i]==1){contimpar=contimpar+1;}}
    }
    if (contpar%2== 1){bitparidadpar=1;}
    if (contimpar%2== 0){bitparidadimpar=1;}
    //...CONSTRUYO DATO WIEGAND
    datawiegand[0]=bitparidadpar;
    datawiegand[25]= bitparidadimpar;
    for(int i=1;i<BITWIEGAND-1;i++){
     datawiegand[i]=binario[i-1];
    }
    //Serial.print(datawiegand[i]);

    //...ENVIO DATO POR D0 Y D1
    for(int i=0;i<BITWIEGAND;i++){
      if(datawiegand[i]==0){
        digitalWrite(PIND0, LOW);
        delayMicroseconds(400);
        digitalWrite(PIND0, HIGH);
        //Serial.print("D0");
      }
      if(datawiegand[i]==1){
        digitalWrite(PIND1, LOW);
        delayMicroseconds(400);
        digitalWrite(PIND1, HIGH);
        //Serial.print("D1");
      }
      delay (2);
    }//delay (2); 
      
}

void setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    yield();
  }
  mySwitch.enableReceive(INDATA);  // Receiver on interrupt 0 => that is pin #2
 // Serial.begin(9600);  
  pinMode(PIND0, OUTPUT);
  pinMode(PIND1, OUTPUT);
 // pinMode(LED_PIN, OUTPUT);
  digitalWrite(PIND0, HIGH);
  digitalWrite(PIND1, HIGH);

 
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
   client.subscribe(subscribetopic);
}

void loop() {
  client.loop();
  if (mySwitch.available()) {
    //......RECIBO CÓDIGO DE LLAVERO
   //digitalWrite(LED_PIN,HIGH);
   valor=mySwitch.getReceivedValue();
   mySwitch.resetAvailable();
   valorstring = String(valor);
   truncstring= valorstring.substring(2);
   numero=truncstring.toInt();
   envioWiegand(numero);
    //digitalWrite(LED_PIN,LOW);
  } //delay(10);
}
