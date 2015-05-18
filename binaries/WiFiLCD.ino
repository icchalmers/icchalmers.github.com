/*
 *  Author: Iain Chalmers
 *  Date: 2015-05-18
 *
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  It is based on the ESP8266WiFi/WiFiWebServer example.
 *
 *  Full details of the hardware setup can be found on my FabAcademy2015 website:
 *
 *    http://fabacademy.org/archives/2015/eu/students/chalmers.iain/week-13-networking.html
 *
 *  Based on the request, the server will:
 *    http://server_ip/LED/on will set GPIO2 LOW
 *    http://server_ip/LED/off will set GPIO2 HIGH
 *    http://server_ip/Line1/{some string} will write {some string} to Line1 of the LCD
 *    http://server_ip/Line2/{some string} will write {some string} to Line2 of the LCD
 *
 *  Remember to set "ssid" and "password" to match your network! On a successful connection,
 *  the IP address of the device will be displayed on the character LCD until new data is 
 *  received.
 */

#include <ESP8266WiFi.h>
#include <Wire.h>
#include "stdio.h"

#define LCD_I2C_ADDRESS 0x08

const char* ssid = "YourSSID";
const char* password = "YourPassword";

char gStringBuffer[101] = {0};

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

const int led = 2;

void setup() {
  // Serial for debug
  Serial.begin(115200);

  // Setup I2C master
  Wire.setClock(400000);
  // GPIO13 is SDA, GPIO 12 is SCL
  Wire.begin(13, 12);

  // LED is ON when GPIO2 is LOW
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WriteToLCD(1, "Connecting to:", 14);
  WriteToLCD(2, (char*)ssid, strlen(ssid));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
   
  // Start the server
  server.begin();
  Serial.println("Server started");
  IPAddress currentIP = WiFi.localIP();
  // Print the IP address
  Serial.println(currentIP);
  
  // Put the device IP on the screen for easy connection
  uint8_t stringLength;
  stringLength = sprintf(gStringBuffer, "Connected. IP:");
  WriteToLCD(1, gStringBuffer, stringLength);
  stringLength = sprintf(gStringBuffer, "%d.%d.%d.%d", currentIP[0], currentIP[1], currentIP[2], currentIP[3]);
  WriteToLCD(2, gStringBuffer, stringLength);
  
}

void loop() {

  uint8_t error = 0;
  String s;

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  if (req.indexOf("/LED/on") != -1) {
    // Prepare the response
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nLED is now on</html>\n";
    digitalWrite(2, 0);
  }
  else if (req.indexOf("/LED/off") != -1) {
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nLED is now off</html>\n";
    digitalWrite(2, 1);
  }
  // If not an LCD command, assume LCD command
  else {
    uint8_t lineNumber;
    bool validLine = false;
    int startIndex = req.indexOf("/") + 1;
    int endIndex = req.indexOf("/", startIndex);
    if (endIndex != -1) {

      String LCDLine = req.substring(startIndex, endIndex);
      Serial.print("LCDLine is: ");
      Serial.println(LCDLine);

      if (LCDLine == "Line1") {
        lineNumber = 1;
        validLine = true;
      }
      if (LCDLine == "Line2") {
        lineNumber = 2;
        validLine = true;
      }
      // Write the input to the correct line of the LCD
      if (validLine) {
        startIndex = endIndex + 1;
        endIndex = req.indexOf(" HTTP/");
        String toLCD = req.substring(startIndex, endIndex);
        // http spaces get replaced with "%20", so turn them back to spaces
        toLCD.replace("%20", " ");
        unsigned int stringLength;
        stringLength = toLCD.length(); // doesn't account for 0 termination
        if (stringLength > 100) {
          stringLength = 100;
        }
        toLCD.getBytes((unsigned char*)gStringBuffer, stringLength + 1); //need to make sure copies the 0 terminator...
        Serial.print("Writing: ");
        Serial.println(gStringBuffer);
        Serial.print("Writing ");
        Serial.print(stringLength + 1);
        Serial.print(" bytes to line number ");
        Serial.println(lineNumber);
        
        error = WriteToLCD(lineNumber, gStringBuffer, stringLength);
        
        if (error) {
          s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nBad I2C write!</html>\n";
        }
        else {
          s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nWrote [";
          s += toLCD + "] to " + LCDLine + "</html>\n";
        }
      } //Invalid HTTP request response
      else {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nInvalid HTTP request!</html>\n";
      }
    }
  }

  client.flush();

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disconnected");

  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}

// Write a string to one line of the LCD display.
//stringLength is the length of the string to write NOT including the trailing /0
int8_t WriteToLCD(uint8_t lineNumber, char* stringBuffer, uint8_t stringLength) {
  
  uint8_t I2CMemoryAddress;
  
  switch (lineNumber) {
    case 1:
        I2CMemoryAddress = 0;
        break;
    case 2:
        I2CMemoryAddress = 100;
        break;
    default:
        return -1;
  }
  Wire.beginTransmission(LCD_I2C_ADDRESS);
  Wire.write(I2CMemoryAddress);
  Wire.write(stringBuffer, stringLength);
  // write spaces to ensure any short string clears the LCD of old content
  if (stringLength < 16) {
    for(uint8_t x=16; x > stringLength; x--){
      Wire.write(' ');
    }
  }
  Wire.write(0); //make sure string is zero terminated
  return Wire.endTransmission();
}
