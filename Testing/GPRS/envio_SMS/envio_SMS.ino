#include <SoftwareSerial.h>
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>


#define MODEM_TX D1
#define MODEM_RX D2
#define SerialMon Serial


SoftwareSerial SerialAT(MODEM_RX, MODEM_TX);

TinyGsm modem(SerialAT);





void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);


  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  // Unlock your SIM card with a PIN if needed
  // modem.simUnlock("1234");
}

void loop() {
  SerialMon.print("Connecting to GSM network... ");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  SerialMon.print("Connecting to GPRS... ");
  if (!modem.gprsConnect("m2m.movistar", "movistar", "movistar")) {
    //  if (!modem.gprsConnect("your_apn", "your_user", "your_password")) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  // Use the modem like a standard AT command modem
  SerialAT.println("AT+CMGF=1");  // Set SMS to text mode
  delay(100);
  SerialAT.println("AT+CMGS=\"+5493415311043\"");  // Replace with your phone number
  delay(100);
  SerialAT.print("Hello from ESP8266");  // Your SMS message
  delay(100);
  SerialAT.write(26);  // Send CTRL+Z to end the SMS

  modem.gprsDisconnect();
  delay(60000);  // Wait a minute before sending next SMS
}
