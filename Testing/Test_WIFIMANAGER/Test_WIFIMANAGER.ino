/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>       // https://github.com/tzapu/WiFiManager


String deviceId = "20000001";
String device_name = "IoT_Cloud_";
String ap_pass = "";
 
// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

String invertirCadena(String s) {
  String temporal = "";
  for (int m = s.length() - 1; m >= 0; m--)
    temporal += s[m];
  return temporal;
}

void setup() {
  Serial.begin(115200);
  
  device_name = device_name + deviceId;
  const char* device_name_char=device_name.c_str();
  ap_pass = invertirCadena(deviceId);
  const char* ap_pass_char=ap_pass.c_str();
  
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,200,1), IPAddress(192,168,200,1), IPAddress(255,255,255,0));
  wifiManager.setMinimumSignalQuality(30);
  wifiManager.setClass("invert"); // dark theme
  wifiManager.setConfigPortalTimeout(180);


  wifiManager.autoConnect(device_name_char, ap_pass_char);
  
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            

            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>IoT Cloud Config tool</h1>");
 
               
             
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
