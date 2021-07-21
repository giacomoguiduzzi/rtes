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
// #include <FS.h>
#include "LittleFS.h"
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

const char* ssid = "Vodafone - Packets Are Coming";
const char* password = "Arouteroficeandfire96!";

uint16_t sensors_delay = FAST;
uint16_t old_sensors_delay = FAST;

sensors_data_t *sensors_data;
uint8_t *sent_data;
bool updating_struct, using_delay;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

unsigned long start_time, end_time;

void setup() {
  // Serial port for debugging purposes (USB Cable)
  Serial.begin(9600);
  Wire.begin();

  delay(1000);

  Serial.println("Setting up data structure.");

  updating_struct = using_delay = false;

  sensors_data = (sensors_data_t *)malloc(sizeof(sensors_data));
  sensors_data->temperature = 0.0;
  sensors_data->humidity = 0.0;
  sensors_data->pressure = 0.0;
  sensors_data->altitude = 0.0;
  sensors_data->brightness = 0.0;

  sent_data = (uint8_t *)sensors_data;

  Serial.println("Setting up LittleFS.");

  // Initialization of LittleFS to read files from FLASH memory
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS. Board stalling.");
    while(1);
  }
  
  Serial.print("Connecting to WiFi..");
  // Initialization of Wi-Fi library and connection to the network
  WiFi.begin(ssid, password);
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

  start_time = end_time = 0;

  Serial.println("Setting up server pages.");
  server_setup();

  Serial.println("Setup complete.");
}

void loop() {
  start_time = millis();

  updating_struct = true;

  Wire.requestFrom(DUE_ADDR, sizeof(sensors_data_t));
  while(!Wire.available());
  for(uint8_t i=0; Wire.available(); i++)
    sent_data[i] = Wire.read();

  updating_struct = false;
  
  MDNS.update();

  end_time = millis();
  int loop_delay = sensors_delay - (end_time - start_time);

  if (loop_delay > 0)
    delay(loop_delay);
}

void server_setup() {
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html");
  });
  server.on("/functions.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/functions.js");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *temp = getTemperature();
    request->send_P(200, "text/plain", temp);
    free(temp);
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *hum = getHumidity();
    request->send_P(200, "text/plain", hum);
    free(hum);
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *pres = getPressure();
    request->send_P(200, "text/plain", pres);
    free(pres);
  });

  server.on("/altitude", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *alt = getAltitude();
    request->send_P(200, "text/plain", alt);
    free(alt);
  });

  server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *brightness = getBrightness();
    request->send_P(200, "text/plain", brightness);
    free(brightness);
  });

  server.on("/delay", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("delay")) {
      AsyncWebParameter *p = request->getParam("delay");
      Serial.print("Received new delay request with argument: ");
      Serial.println(p->value());
      char * ok = setDelay(p->value());
      request->send_P(200, "text/plain", ok);
      free(ok);
    }
  });

  server.on("/getdelay", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *current_delay = getDelay();
    request->send_P(200, "text/plain", current_delay);
    free(current_delay);
  });

  // Start server
  server.begin();
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
