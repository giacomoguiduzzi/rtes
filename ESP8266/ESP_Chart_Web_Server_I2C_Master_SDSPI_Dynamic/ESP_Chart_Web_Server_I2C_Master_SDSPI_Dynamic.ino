/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <Wire.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <SD.h>
#include <ESP8266mDNS.h>


#define DELAY_FLAG 0x64 // "d" letter
// #define I2C_ADDR 0x33
#define DUE_ADDR 0x33

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

char* ssid;
char* password;

uint16_t sensors_delay = FAST;
uint16_t old_sensors_delay = FAST;

sensors_data_t *sensors_data;
uint8_t *sent_data;
bool updating_struct, using_delay;

// Create AsyncWebServer object on port 80
AsyncWebServer *server;

unsigned long *start_time;
unsigned long *end_time;
int *loop_delay;

/* extern "C" {
#include "user_interface.h"
} */

void setup() {
  // Serial port for debugging purposes (USB Cable)
  Serial.begin(9600);
  Wire.begin();

  delay(1000);

  ssid = (char *)malloc(sizeof(char) * 30);
  password = (char *)malloc(sizeof(char) * 23);
  server = (AsyncWebServer *)malloc(sizeof(AsyncWebServer));
  start_time = (unsigned long *)malloc(sizeof(unsigned long));
  end_time = (unsigned long *)malloc(sizeof(unsigned long));
  loop_delay = (int *)malloc(sizeof(int));

  
  sprintf(ssid, "%s", "Vodafone - Packets Are Coming");
  sprintf(password, "%s", "Arouteroficeandfire96!");

  Serial.println("Setting up data structure.");

  updating_struct = using_delay = false;

  *server = AsyncWebServer(80);

  sensors_data = (sensors_data_t *)malloc(sizeof(sensors_data));
  sensors_data->temperature = 0.0;
  sensors_data->humidity = 0.0;
  sensors_data->pressure = 0.0;
  sensors_data->altitude = 0.0;
  sensors_data->brightness = 0.0;

  sent_data = (uint8_t *)sensors_data;

  Serial.println("Setting up SD Card.");

  // Initialization of LittleFS to read files from FLASH memory
  if (!SD.begin(15)) {
    Serial.println("Couldn't initialize the SD Reader. Board stalling.");
    while(1)
      delay(1000);
  }

  if(!SD.exists("/index.html") || !SD.exists("/functions.js")){
    Serial.println("It looks like some files are missing on the SD Card. Board stalling.");
    while(1)
      delay(1000);  
  }

  /* uint32_t free_ = system_get_free_heap_size();
  Serial.print("Free Heap before Wi-Fi connection: ");
  Serial.println(free_); */
  
  Serial.print("Connecting to WiFi..");
  // Initialization of Wi-Fi library and connection to the network
  // WiFi.mode(WIFI_STA);
  // WiFi.persistent(false);
  WiFi.begin(ssid, password);
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }

  Serial.print("\nIP Address: ");
  Serial.println(WiFi.localIP());

  // Initialization of the DNS library and set-up of the host name
  if (MDNS.begin("weatherstation"))
    Serial.println("DNS okay. You can connect to the station via the address: http://weatherstation.local");
  else
    Serial.println("There was a problem with the DNS initialization but you can still connect via the IP address.");

  *start_time = 0;
  *end_time = 0;

  Serial.println("Setting up server pages.");
  server_setup();

  Serial.println("Setup complete.");
}

void loop() {
  *start_time = millis();

  updating_struct = true;

  Wire.requestFrom(DUE_ADDR, sizeof(sensors_data_t));
  while(!Wire.available());
  for(uint8_t i=0; Wire.available(); i++)
    sent_data[i] = Wire.read();

  updating_struct = false;
  
  MDNS.update();

  *end_time = millis();
  *loop_delay = sensors_delay - (end_time - start_time);

  if (*loop_delay > 0)
    delay(*loop_delay);
}

void server_setup() {
  // Route for root / web page
  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    File index_file = SD.open("/index.html");
    String *index_str = (String *)malloc(sizeof(String) * index_file.size());
    
    while(index_file.available())
      *index_str += (char)index_file.read();

    index_file.close();
    
    request->send(200, *index_str);
    
    free(index_str);
  });
  
  server->on("/functions.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    File functions_file = SD.open("/functions.js");
    String *functions_str = (String *)malloc(sizeof(String) * functions_file.size());

    while(functions_file.available())
      *functions_str += (char)functions_file.read();

    functions_file.close();

    request->send(200, *functions_str);

    free(functions_str);
  });
  
  server->on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *temp = getTemperature();
    request->send_P(200, "text/plain", temp);
    free(temp);
  });
  
  server->on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *hum = getHumidity();
    request->send_P(200, "text/plain", hum);
    free(hum);
  });
  
  server->on("/pressure", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *pres = getPressure();
    request->send_P(200, "text/plain", pres);
    free(pres);
  });

  server->on("/altitude", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *alt = getAltitude();
    request->send_P(200, "text/plain", alt);
    free(alt);
  });

  server->on("/brightness", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *brightness = getBrightness();
    request->send_P(200, "text/plain", brightness);
    free(brightness);
  });

  server->on("/delay", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("delay")) {
      AsyncWebParameter *p = request->getParam("delay");
      Serial.print("Received new delay request with argument: ");
      Serial.println(p->value());
      char * ok = setDelay(p->value());
      request->send_P(200, "text/plain", ok);
      free(ok);
    }
  });

  server->on("/getdelay", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *current_delay = getDelay();
    request->send_P(200, "text/plain", current_delay);
    free(current_delay);
  });

  // Start server
  server->begin();
}

char *getTemperature() {
  char *temp = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  //while(!digitalRead(SSPIN));
  // TODO: check if this freezes because of a request from client during struct update
  while(updating_struct);
  sprintf(temp, "%.02f", sensors_data->temperature);
  // Serial.print("Sending temperature value to client: ");
  // Serial.println(String(temp));
  return temp;
}

char *getHumidity() {
  char *hum = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(updating_struct);
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
  union{
    uint16_t uint16;
    uint8_t uint8[2];
  } new_delay;

  new_delay.uint16 = strtol(delay_.c_str(), NULL, DEC);

  Serial.print("New delay in setDelay(): ");
  Serial.println(new_delay.uint16);

  if(new_delay.uint16 == sensors_delay)
    sprintf(result, "ok");
  else
  {
    old_sensors_delay = sensors_delay;
    bool answer = set_new_delay(new_delay.uint16);
  
    if (answer) {
      Wire.beginTransmission(DUE_ADDR);
      Wire.write(new_delay.uint8[0]);
      Wire.write(new_delay.uint8[1]);
      Wire.endTransmission();

      Wire.requestFrom(DUE_ADDR, 3);
      while(!Wire.available());
      char *answer = (char *)malloc(sizeof(char) * 3);

      for(uint8_t i=0; Wire.available(); i++)
        answer[i] = Wire.read();

      if(strcmp(answer, "ok\0") == 0){
        sprintf(result, "ok");
        old_sensors_delay = sensors_delay;
      }
      else{
        sprintf(result, "no");
        sensors_delay = old_sensors_delay;
      }
    }
    else
      sprintf(result, "no");
  
    Serial.print("Sending back response \"");
    Serial.print(result);
    Serial.println("\" to client.");
  }

  return result;
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
