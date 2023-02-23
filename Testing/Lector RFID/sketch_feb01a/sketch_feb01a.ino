//Programa: Leitor RFID RDM6300 com ESP8266 NodeMCU
//Autor: Arduino e Cia

#include <SoftwareSerial.h>
#include <RDM6300.h>

//Inicializa a serial nos pinos 12 (RX) e 13 (TX) 
SoftwareSerial RFID(12, 13);

//Configura o led na porta digital D2
int Led = 4;

uint8_t Payload[6]; // used for read comparisons

RDM6300 RDM6300(Payload);

void setup()
{
  //Define o pino do led como saida
  pinMode(Led, OUTPUT);
  
  //Inicializa a serial para o leitor RDM6300
  RFID.begin(9600);
  
  //Inicializa a serial para comunicacao com o PC
  Serial.begin(9600);
  
  //Informacoes iniciais
  Serial.println("Leitor RFID RDM6300");
}

void loop()
{
  //Apaga o led
  digitalWrite(Led, HIGH);
  
  //Aguarda a aproximacao da tag RFID
  while (RFID.available() > 0)
  {
    
    uint8_t c = RFID.read();
    if (RDM6300.decode(c))
    {
      Serial.print("ID TAG: ");
      //Mostra os dados no serial monitor
      for (int i = 0; i < 5; i++) {
        Serial.print(Payload[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      //Aciona o led
      digitalWrite(Led, LOW);
    }
  }

  //Aguarda 2 segundos e reinicia o processo
  delay(500);
}
