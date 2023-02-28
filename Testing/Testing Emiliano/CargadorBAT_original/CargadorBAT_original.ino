// ENTRADAS:
#define VBAT_POS 36         // Mide tensión en terminal positivo de la batería 
#define VBAT_NEG 39         // Mide tensión en terminal negativo de la batería
#define IBAT 34             // Mide corriente de carga y descarga de la batería
// SALIDAS:
#define PWM 23
#define RELE_SAL 22
#define RELE_CARGA 21

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
int valorPWM = 511;            // Con PWM máximo el MOSFET arranca apagado (está invertido por transistor de acople)
#define CANAL 0            
#define PREC 9                 // Precisión de 9 bits implica valores entre 0 y 511
#define FREC 5000              // Frecuencia en HZ

//Variables y Constantes de Medicion de Tension:
#define V_MAX 14.4             //Máxima tensión de carga de la batería
#define V_CORTE 11             //Tensión a la que se corta la salida para protección por sobredescarga  
#define divisor_1  0.19     // Coeficiente del divisor de tensíon = R2/(R1+R2)
#define divisor_2  0.18248
float tensionBAT = 0;          // Tensión de Batería = (VBAT_POS - VBAT_NEG)
float tensionBAT_POS = 0;      // Es VCC
float tensionBAT_NEG = 0;      // En descarga es 0 y en carga puede ser mayor que cero

//Funcion Setup:
void setup(){
   Serial.begin(115200);
   ledcSetup(CANAL,FREC,PREC);
   ledcAttachPin(PWM,CANAL);
   valorPWM = 128;
   ledcWrite(CANAL, valorPWM);
   delay(500);
}

//Funcion Loop:
void loop(){
   I_BAT = mideCorrienteBAT();
   Serial.print("I_BAT: ");
   Serial.println(I_BAT,3);
   tensionBAT_POS = mideTension(VBAT_POS, divisor_1); 
   Serial.print("VBAT_POS: ");
   Serial.println(tensionBAT_POS,3);
   tensionBAT_NEG = mideTension(VBAT_NEG, divisor_2); 
   Serial.print("VBAT_NEG: ");
   Serial.println(tensionBAT_NEG,3);
   tensionBAT = tensionBAT_POS - tensionBAT_NEG;
   Serial.print("Tensión Batería: ");
   Serial.println(tensionBAT,3);
   ajustaPWMcargador();
   Serial.println(" ");
   Serial.println(" ");
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
      } 
      else{
         Serial.println("Límite de Tensión");        
         if(valorPWM<511)
            valorPWM++;
         delay(1);
         ledcWrite(CANAL, valorPWM);
         Serial.print("PWM: ");
         Serial.println(valorPWM); 
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
   }
}   