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
// #include "FS.h"
// #include "LittleFS.h"
#include <Wire.h>
#include <Effortless_SPIFFS.h>

inline bool checkFlashConfig();

#define DELAY_FLAG 0x64 // "d" letter
#define I2C_ADDR 0x33

typedef struct {
  float temperature;
  float pressure;
  float humidity;
  float altitude;
  float brightness;
} sensors_data_t;

typedef enum
{
  FAST = 500,
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

ESP8266WebServer server(80);
// const uint8_t pin_led = 2;
const char* ssid = "Vodafone - Packets Are Coming";
const char* password = "Arouteroficeandfire96!";

uint16_t sensors_delay = FAST;
uint16_t old_sensors_delay = FAST;

sensors_data_t *sensors_data;
uint8_t *sent_data;
bool updating_struct, using_delay;

unsigned long start_time, end_time, loop_delay;

eSPIFFS fileSystem;

void setup()
{
  Wire.begin(I2C_ADDR);
  Wire.onReceive(readSensorData);
  Wire.onRequest(sendNewDelay);
  
  // pinMode(pin_led, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.begin(9600);
  while(!Serial);

  updating_struct = using_delay = false;

  sensors_data = (sensors_data_t *)malloc(sizeof(sensors_data));
  sensors_data->temperature = 0.0;
  sensors_data->humidity = 0.0;
  sensors_data->pressure = 0.0;
  sensors_data->altitude = 0.0;
  sensors_data->brightness = 0.0;

  sent_data = (uint8_t *)sensors_data;

  start_time = end_time = loop_delay = 0;
  
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

  // LittleFS.begin();
  /*if(!LittleFS.begin())
    Serial.println("An error has occured while mounting SPIFFS");*/

  /*bool spiffsSetCorrectly = fileSystem.checkFlashConfig();
  
  if(!spiffsSetCorrectly){
    Serial.println("There was an error during filesystem configuration.");
    while(1);
  }*/

  // server.on("/", sendHomePage);
  // server.on("/ledstate",getLEDState);

  server.on("/", [](){
    /*File index = LittleFS.open("/index.html", "r");
    server.send_P(200, "text/html", index.readString().c_str());
    index.close();*/
    size_t fileSize = fileSystem.getFileSize("/index.html");
    char *fileContents = (char *)malloc(sizeof(char) * (fileSize + 1));  // Dont forget about the null terminator for C Strings
    fileSystem.openFile("/index.html", fileContents, fileSize);
    server.send_P(200, "text/html", fileContents);
    free(fileContents);
  });
  
  server.on("/functions.js", [](){
    /*File functions = LittleFS.open("/functions.js", "r");
    server.send_P(200, "text/html", functions.readString().c_str());
    functions.close();*/
    size_t fileSize = fileSystem.getFileSize("/functions.js");
    char *fileContents = (char *)malloc(sizeof(char) * (fileSize + 1));  // Dont forget about the null terminator for C Strings
    fileSystem.openFile("/functions.js", fileContents, fileSize);
    server.send_P(200, "text/javascript", fileContents);
    free(fileContents);
  });

  /* File file = LittleFS.open("/index.html", "r");

  if(!file)
    Serial.println("Failed to open homepage.html file for reading");

  Serial.println("File content: ");
  while(file.available())
    Serial.write(file.read());

  file.close();*/
  
  // server.on("/", sendHomePage);
  // server.on("/ledstate",getLEDState);
  server.on("/temperature", [](){
    char *temp = getTemperature();
    server.send_P(200, "text/plain", temp);
    free(temp);
  });
  server.on("/humidity", [](){
    char *hum = getHumidity();
    server.send_P(200, "text/plain", hum);
    free(hum);
  });
  server.on("/pressure", []() {
    char *pres = getPressure();
    server.send_P(200, "text/plain", pres);
    free(pres);
  });

  server.on("/altitude", [](){
    char *alt = getAltitude();
    server.send_P(200, "text/plain", alt);
    free(alt);
  });

  server.on("/brightness", []() {
    char *brightness = getBrightness();
    server.send_P(200, "text/plain", brightness);
    free(brightness);
  });

  /* server.on("/delay", [](AsyncWebServerRequest * request) {
    if (request->hasParam("delay")) {
      AsyncWebParameter *p = request->getParam("delay");
      Serial.print("Received new delay request with argument: ");
      Serial.println(p->value());
      char * ok = setDelay(p->value());
      request->send_P(200, "text/plain", ok);
      free(ok);
    }
  });*/

  server.on("/getdelay", []() {
    char *current_delay = getDelay();
    server.send_P(200, "text/plain", current_delay);
    free(current_delay);
  });
  
  server.begin();
}

void loop()
{
  server.handleClient();
  MDNS.update();
}

char *getTemperature() {
  char *temp = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  //while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(temp, "%.02f", sensors_data->temperature);
  // Serial.print("Sending temperature value to client: ");
  // Serial.println(String(temp));
  return temp;
}

char *getHumidity() {
  char *hum = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(hum, "%.02f", sensors_data->humidity);
  // Serial.print("Sending humidity value to client: ");
  // Serial.println(String(hum));
  return hum;
}

char *getPressure() {
  char *pres = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(pres, "%.02f", sensors_data->pressure);
  // Serial.print("Sending pressure value to client: ");
  //Serial.println(String(pres));
  return pres;
}

char *getAltitude() {
  char *alt = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(alt, "%.02f", sensors_data->altitude);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return alt;
}

char *getBrightness() {
  char *brightn = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(brightn, "%.02f", sensors_data->brightness);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return brightn;
}

char *getDelay(){
  char *current_delay;

  switch(sensors_delay){
    case FAST:
      current_delay = (char *)malloc(sizeof(char) * 4);
      break;

    case MEDIUM:
    case SLOW:
    case VERY_SLOW:
      current_delay = (char *)malloc(sizeof(char) * 5);
      break;

    case TAKE_A_BREAK:
      current_delay = (char *)malloc(sizeof(char) * 6);
      break;

    default:
      Serial.println("Something wrong is getDelay function.");
      current_delay = (char *)malloc(sizeof(char) * 6);
      sprintf(current_delay, "error");
    break;
  }

  sprintf(current_delay, "%u", sensors_delay);

  return current_delay;
}

char *setDelay(String delay_) {
  char *result = (char *)malloc(sizeof(char) * 3);
  uint16_t new_delay = strtol(delay_.c_str(), NULL, DEC);

  Serial.print("New delay in setDelay(): ");
  Serial.println(new_delay);

  if(new_delay == sensors_delay)
    sprintf(result, "ok");
  else
  {
    old_sensors_delay = sensors_delay;
    bool answer = set_new_delay(new_delay);
  
    if (answer) {
      sprintf(result, "ok");
    }
    else
      sprintf(result, "no");
  
    Serial.print("Sending back response \"");
    Serial.print(result);
    Serial.println("\" to client.");
  }

  return result;
}

/*void sendHomePage(){
  
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
}*/

void sendNewDelay() {
  Serial.println("Sending new delay to Arduino Due.");
  using_delay = true;
  Wire.write(sensors_delay);
  using_delay = false;
}

bool set_new_delay(const uint16_t new_delay) {
  bool ok = true;

  switch (new_delay) {
    case 500:
      sensors_delay = FAST;
      break;
    case 1000:
      sensors_delay = MEDIUM;
      break;
    case 2500:
      sensors_delay = SLOW;
      break;
    case 5000:
      sensors_delay = VERY_SLOW;
      break;
    case 10000:
      sensors_delay = TAKE_A_BREAK;
      break;
    default:
      Serial.print("There was an error while setting new delay: ");
      Serial.println(new_delay);
      ok = false;
      break;
  }

  return ok;
}

void readSensorData(int bytes_to_read) {
  Serial.println("Receiving data from Arduino Due.");
  
  uint8_t struct_size = sizeof(sensors_data_t);

  if(bytes_to_read == 4){
    using_delay = true;
    
    uint8_t delay_flag = Wire.read();

    if(delay_flag == DELAY_FLAG){
      char *answer = (char *)malloc(sizeof(char) * 3);
  
      for(uint8_t i = 0; i < 3; i++)
        answer[i] = Wire.read();
  
      if(strcmp(answer, "ok\0") == 0){
        Serial.println("New delay confirmed and set up.");
        old_sensors_delay = sensors_delay;
      }
  
      else if(strcmp(answer, "no\0") == 0){
        Serial.println("Something went wrong with the new delay, received a \"no\" answer. Going back");
        sensors_delay = old_sensors_delay;
      }
  
      else{
        Serial.println("Received answer is neither yes nor no. What now?");
        // Assuming no
        sensors_delay = old_sensors_delay;
      }
    }
    else{
      Serial.println("Waiting for a new-delay-set answer, the first byte sent as an answer is not the DELAY_FLAG. Aborting change");
      sensors_delay = old_sensors_delay;  
    }

    using_delay = false;
  }
  else{
    updating_struct = true;
    
    Serial.println("Receiving new data struct.");
  
    if (bytes_to_read != struct_size) {
      Serial.println("Sent data structure size doesn't correspond, thus it hasn't been read.");
      return;
    }
  
    for (uint8_t i = 0; i < sizeof(sensors_data); i++) {
      sent_data[i] = Wire.read();
    }
  
    updating_struct = false;
  
    Serial.print("Temperature: ");
    Serial.print(sensors_data->temperature);
    Serial.println("°C");
  
    Serial.print("(HEX: ");
    Serial.print(sent_data[0], HEX);
    Serial.print(sent_data[1], HEX);
    Serial.print(sent_data[2], HEX);
    Serial.print(sent_data[3], HEX);
    Serial.println(")");
  
    Serial.print("Pressure: ");
    Serial.print(sensors_data->pressure);
    Serial.println(" hPa");
  
    Serial.print("Humidity: ");
    Serial.print(sensors_data->humidity);
    Serial.println("%");
  
    Serial.print("Altitude: ");
    Serial.print(sensors_data->altitude);
    Serial.println("m");
  
    Serial.print("Brightness: ");
    Serial.print(sensors_data->brightness);
    Serial.println(" lux");
    Serial.println();
  }
}
