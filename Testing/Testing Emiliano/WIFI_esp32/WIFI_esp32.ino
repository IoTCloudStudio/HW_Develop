/*
 * El ejemplo de esta semana es una conexión sencilla hacia alguna red WiFi 
 * 
 * Para esta semana, necesitamos lo siguiente:
 * ESP32
 * Una red WiFi existente en nuestro hogar, lugar de trabajo, etc.
 * 
 * No habrá una conexión física con ningún componente extra.
 * Realizado por: Sebastian Carranza C. -  TodoMaker
 */

/*Incluimos primero la librería*/
#include <WiFi.h>

/*Definimos nuestras credenciales de la red WiFi*/
const char* ssid = "Fibertel WiFi288 2.4GHz";
const char* pass = "0143512984";

void setup() {
  /*Iniciamos el terminal Serial para la escritura de algunos parámetros */
  Serial.begin(115200);
  /*Iniciamos la conexión a la red WiFi definida*/
  WiFi.begin(ssid, pass);
  delay(2000);
  /*En el terminal Serial, indicamos que se está realizando la conexión*/
  Serial.print("Se está conectando a la red WiFi denominada ");
  Serial.println(ssid);
  /*Mientras se realiza la conexión a la red, aparecerán puntos, hasta que se realice la conexión*/
  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
  /*Con la conexión ya realizada, se pasa a imprimir algunos datos importantes, como la dirección IP asignada a nuestro dispositivo*/
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  /*No se contempla el uso de un loop en el ejemplo de esta semana*/
}