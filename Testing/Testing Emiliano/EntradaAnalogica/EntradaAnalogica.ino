/*
  Lee una tensión analógica por el pin A0 y la compara con un valor mínimo Vmin y máximo Vmax de tensión para actuar sobre una salida.
  De esta forma la salida se mantiene en alto siempre que la tensíon medida esté en un rango seguro para la carga.
*/
float Vmin = 10; // Tensión mínima a la que queremos que corte la salida
float Varranque = 11; // Tensión de reestablecimiento
float Vmax = 15; // Tensión máxima a la que queremos que corte la salida
float Vcc;
const byte pinSalida = 5; //Elijo qué numero de pin voy a usar para activar la salida (D1)
const int Rsup = 6820;
const int Rinf = 1212;
float error = 0.4;

void setup() {

  Serial.begin(115200);
  pinMode(pinSalida,OUTPUT);
}

void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);  
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.3V):
  float voltage = sensorValue * (3.3 / 1023.0);
  //Calculo la Vcc con la tensión medida y el divisor de tensión formado por Rsup y Rinf:
  Vcc = voltage*(Rinf + Rsup)/Rinf - error;
  
 //Armo el comparador:
 if (Vcc <= Vmin ) {
    digitalWrite(pinSalida, LOW);
 }
  if (Vcc >= Varranque) {
    digitalWrite(pinSalida, HIGH);  
    }
  if (Vcc >= Vmax) {
    digitalWrite(pinSalida, LOW);    
  }
  
 Serial.println(Vcc);
  delay (100);
} 
