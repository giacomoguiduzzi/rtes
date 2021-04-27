/*------------------------------------------------------------------------------
  07/01/2018
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: webserver_html_js.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to use JavaScript in HTML weppages
  that are served in a web server's response.
  https://youtu.be/ZJoBy2c1dPk

  Do you like my videos? You can support the channel:
  https://patreon.com/acrobotic
  https://paypal.me/acrobotic
  ------------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 

  https://acrobotic.com/
  https://amazon.com/acrobotic
  ------------------------------------------------------------------------------
  License:
  Please see attached LICENSE.txt file for details.
------------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"

ESP8266WebServer server(80);
const uint8_t pin_led = 2;
char* ssid = "Vodafone - Packets Are Coming";
char* password = "Arouteroficeandfire96!";

void setup()
{
  pinMode(pin_led, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.begin(9600);
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  if(MDNS.begin("weatherstation")){
    Serial.println("DNS okay! You can connect to http://weatherstation.local");
  }

  if(!SPIFFS.begin())
    Serial.println("An error has occured while mounting SPIFFS");

  File file = SPIFFS.open("/homepage.html", "r");

  if(!file)
    Serial.println("Failed to open homepage.html file for reading");

  Serial.println("File content: ");
  while(file.available())
    Serial.write(file.read());

  file.close();
  
  server.on("/", sendHomePage);
  server.on("/ledstate",getLEDState);
  server.begin();
}

void loop()
{
  server.handleClient();
  MDNS.update();
}

void sendHomePage(){
  
  server.send_P(200,"text/html", "");
}

void toggleLED()
{
  digitalWrite(pin_led,!digitalRead(pin_led));
}

void getLEDState()
{
  toggleLED();
  String led_state = digitalRead(pin_led) ? "OFF" : "ON";
  server.send(200,"text/plain", led_state);
}
