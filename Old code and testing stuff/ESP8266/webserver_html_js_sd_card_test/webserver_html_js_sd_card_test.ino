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
// #include <SPI.h>
// #include <SD.h>
#include <Wire.h>

#define DELAY_FLAG 0x64 // "d" letter
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

ESP8266WebServer server(80);
// const uint8_t pin_led = 2;
const char* ssid = "Vodafone - Packets Are Coming";
const char* password = "Arouteroficeandfire96!";

uint16_t sensors_delay = FAST;
uint16_t old_sensors_delay = FAST;

sensors_data_t *sensors_data;
uint8_t *sent_data;

unsigned long start_time, end_time, loop_delay;

void setup()
{
  Wire.begin();
  
  // pinMode(pin_led, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.begin(9600);
  while(!Serial);

  // updating_struct = using_delay = false;

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

  if(MDNS.begin("weatherstation"))
    Serial.println("DNS okay! You can connect to http://weatherstation.local");
  else
    Serial.println("There was a problem with the DNS initialization but you can still connect via the IP address.");

  // Initialization of SD to read files from
  /*if (!SD.begin(15)) {
    Serial.println("Couldn't initialize the SD Reader. Board stalling.");
    while(1)
      delay(1000);
  }

  if(!SD.exists("/index.html") || !SD.exists("/functions.js")){
    Serial.println("It looks like some files are missing on the SD Card. Board stalling.");
    while(1)
      delay(1000);  
  } */

  server.on("/", [](){
    /*String index_str = "";
    File index_file = SD.open("/index.html");
    
    while(index_file.available())
      index_str += (char)index_file.read();

    index_file.close();
    
    server.send_P(200, "text/html", index_str.c_str());*/
    server.send_P(200, "text/html", "ciao");
  });
  
  server.on("/functions.js", [](){
    /*String functions_str = "";
    File functions_file = SD.open("/functions.js");

    while(functions_file.available())
      functions_str += (char)functions_file.read();

    functions_file.close();
    
    server.send_P(200, "text/javascript", functions_str.c_str());*/
    server.send_P(200, "text/javascript", "<script>alert(1);</script>");
  });

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

  server.on("/delay", []() {
    if(server.arg("delay")=="")
      Serial.println("ERROR: For some reason the set-new-delay request has 0 arguments.");
    else{
      String new_delay = server.arg("delay");
      Serial.println("Received new delay request with argument: " + new_delay);
      char * ok = setDelay(new_delay);
      server.send_P(200, "text/plain", ok);
      free(ok);
    }
  });

  server.on("/getdelay", []() {
    char *current_delay = getDelay();
    server.send_P(200, "text/plain", current_delay);
    free(current_delay);
  });
  
  server.begin();
}

void loop()
{
  start_time = millis();
  
  server.handleClient();
  MDNS.update();

  Wire.requestFrom(DUE_ADDR, sizeof(sensors_data_t));
  while(!Wire.available());
  for(uint8_t i=0; Wire.available(); i++)
    sent_data[i] = Wire.read();

  end_time = millis();
  
  loop_delay = sensors_delay - (end_time - start_time);
  if (loop_delay > 0)
    delay(loop_delay);
}

char *getTemperature() {
  char *temp = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  //while(!digitalRead(SSPIN));
  //while (updating_struct);
  sprintf(temp, "%.02f", sensors_data->temperature);
  // Serial.print("Sending temperature value to client: ");
  // Serial.println(String(temp));
  return temp;
}

char *getHumidity() {
  char *hum = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  //while (updating_struct);
  sprintf(hum, "%.02f", sensors_data->humidity);
  // Serial.print("Sending humidity value to client: ");
  // Serial.println(String(hum));
  return hum;
}

char *getPressure() {
  char *pres = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  //while (updating_struct);
  sprintf(pres, "%.02f", sensors_data->pressure);
  // Serial.print("Sending pressure value to client: ");
  //Serial.println(String(pres));
  return pres;
}

char *getAltitude() {
  char *alt = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  //while (updating_struct);
  sprintf(alt, "%.02f", sensors_data->altitude);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return alt;
}

char *getBrightness() {
  char *brightn = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  //while (updating_struct);
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
      Serial.println("Something wrong in getDelay function.");
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
