#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();
#define PIND0 2
#define PIND1 0
#define INDATA 3   
long valor;
long bit;
String valorstring;
String truncstring;
int numero;
#define CANTBITCODE 24 // 26 BIT FORMATO WIEGAND MENOS 2 BIT DE PARIDAD
#define BITWIEGAND 26 // 26 BIT FORMATO WIEGAND
int binario[CANTBITCODE];
int bitparidadpar;
int bitparidadimpar;
int contpar = 0;
int contimpar = 0;
int datawiegand [BITWIEGAND];
bool recibidato = false;

void setup() {
  mySwitch.enableReceive(INDATA);  // Receiver on interrupt 0 => that is pin #2
  //Serial.begin(9600);  
  pinMode(PIND0, OUTPUT);
  pinMode(PIND1, OUTPUT);
  digitalWrite(PIND0, HIGH);
  digitalWrite(PIND1, HIGH);
}

void loop() {
  if (recibidato){
    delay (2000);
    recibidato = false;
    mySwitch.resetAvailable();
  }
  if (mySwitch.available()) {
    //......RECIBO CÓDIGO DE LLAVERO
   valor=mySwitch.getReceivedValue();
   valorstring = String(valor);
   truncstring= valorstring.substring(2);
   numero=truncstring.toInt();
   //Serial.println(valorstring);
   //Serial.println(truncstring);
  
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
    }delay (100); 
    recibidato = true;
  } delay(10);
}
