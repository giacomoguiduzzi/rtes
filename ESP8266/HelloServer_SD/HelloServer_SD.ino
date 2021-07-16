#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SD.h>
#include <Wire.h>

#ifndef STASSID
#define STASSID "Vodafone - Packets Are Coming"
#define STAPSK  "Arouteroficeandfire96!"
#endif

#define DUE_ADDR 0x33
#define ISR_BUTTON_PIN 2

const char* ssid = STASSID;
const char* password = STAPSK;

typedef struct {
  float temperature;
  float pressure;
  float humidity;
  float altitude;
  float brightness;
} sensors_data_t;

typedef struct{
  char temperature[5];
  char pressure[5];
  char humidity[5];
  char altitude[5];
  char brightness[5];
} sensors_data_str_t;

typedef enum
{
  FAST = 500,
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

uint16_t sensors_delay = FAST;
uint16_t old_sensors_delay = FAST;

sensors_data_t *sensors_data;
sensors_data_str_t sensors_data_str;
uint8_t *sent_data;

ESP8266WebServer server(80);

unsigned long start_time, end_time, loop_delay;

void handleRoot() {
  // server.send(200, "text/plain", "hello from esp8266!\r\n");
  Serial.println("Received request for index page.");
  
  File index_file = SD.open("/index.html");
  
  size_t sent = server.streamFile(index_file, "text/html");
  
  if(sent != index_file.size())
    Serial.println("Sent less bytes to client than expected for the index page.");
  
  index_file.close();

  Serial.println("Page index.html sent.");
}

void handleJS(){
  Serial.println("Received request for JS file.");
  
  File functions_file = SD.open("/functions.js");

  size_t sent = server.streamFile(functions_file, "text/javascript");

  if(sent != functions_file.size())
    Serial.println("Sent less bytes to client than expected for the functions.js file");

  functions_file.close();

  Serial.println("JS sent.");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(9600);
  while(!Serial)
    delay(100);

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

  Serial.println("SD initialized.");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  sensors_data = (sensors_data_t *)malloc(sizeof(sensors_data));
  /*sensors_data->temperature = 0;
  sensors_data->humidity = 0;
  sensors_data->pressure = 0;
  sensors_data->altitude = 0;
  sensors_data->brightness = 0;*/

  sent_data = (uint8_t *)sensors_data;

  start_time = end_time = loop_delay = 0;

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("weatherstation")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/functions.js", handleJS);

  server.on("/temperature", getTemperature);
  server.on("/humidity", getHumidity);
  server.on("/pressure", getPressure);
  server.on("/altitude", getAltitude);
  server.on("/brightness", getBrightness);
  server.on("/delay", setDelay);

  server.on("/getdelay", getDelay);

  /*server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/gif", []() {
    static const uint8_t gif[] PROGMEM = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80, 0x01,
      0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x19, 0x8c, 0x8f, 0xa9, 0xcb, 0x9d,
      0x00, 0x5f, 0x74, 0xb4, 0x56, 0xb0, 0xb0, 0xd2, 0xf2, 0x35, 0x1e, 0x4c,
      0x0c, 0x24, 0x5a, 0xe6, 0x89, 0xa6, 0x4d, 0x01, 0x00, 0x3b
    };
    char gif_colored[sizeof(gif)];
    memcpy_P(gif_colored, gif, sizeof(gif));
    // Set the background to a random set of colors
    gif_colored[16] = millis() % 256;
    gif_colored[17] = millis() % 256;
    gif_colored[18] = millis() % 256;
    server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
  }); */

  server.onNotFound(handleNotFound);

  /////////////////////////////////////////////////////////
  // Hook examples

  /* server.addHook([](const String & method, const String & url, WiFiClient * client, ESP8266WebServer::ContentTypeFunction contentType) {
    (void)method;      // GET, PUT, ...
    (void)url;         // example: /root/myfile.html
    (void)client;      // the webserver tcp client connection
    (void)contentType; // contentType(".html") => "text/html"
    Serial.printf("A useless web hook has passed\n");
    Serial.printf("(this hook is in 0x%08x area (401x=IRAM 402x=FLASH))\n", esp_get_program_counter());
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String & url, WiFiClient*, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/fail")) {
      Serial.printf("An always failing web hook has been triggered\n");
      return ESP8266WebServer::CLIENT_MUST_STOP;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String & url, WiFiClient * client, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/dump")) {
      Serial.printf("The dumper web hook is on the run\n");

      // Here the request is not interpreted, so we cannot for sure
      // swallow the exact amount matching the full request+content,
      // hence the tcp connection cannot be handled anymore by the
      // webserver.
#ifdef STREAMSEND_API
      // we are lucky
      client->sendAll(Serial, 500);
#else
      auto last = millis();
      while ((millis() - last) < 500) {
        char buf[32];
        size_t len = client->read((uint8_t*)buf, sizeof(buf));
        if (len > 0) {
          Serial.printf("(<%d> chars)", (int)len);
          Serial.write(buf, len);
          last = millis();
        }
      }
#endif
      // Two choices: return MUST STOP and webserver will close it
      //                       (we already have the example with '/fail' hook)
      // or                  IS GIVEN and webserver will forget it
      // trying with IS GIVEN and storing it on a dumb WiFiClient.
      // check the client connection: it should not immediately be closed
      // (make another '/dump' one to close the first)
      Serial.printf("\nTelling server to forget this connection\n");
      static WiFiClient forgetme = *client; // stop previous one if present and transfer client refcounter
      return ESP8266WebServer::CLIENT_IS_GIVEN;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  }); */

  // Hook examples
  /////////////////////////////////////////////////////////

  server.begin();
  Serial.println("HTTP server started");

  Wire.begin();

  Serial.println("Wire library initialized.");

  // ISR setup
  pinMode(ISR_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ISR_BUTTON_PIN), buttonDelay, RISING);

}

void loop(void) {
  //start_time = millis();
  
  server.handleClient();
  MDNS.update();

  // updateStruct();

  //end_time = millis();

  /* int loop_delay = sensors_delay - (end_time - start_time);

  if(loop_delay > 0)
    delay(loop_delay);*/
}

ICACHE_RAM_ATTR void buttonDelay(){
  
switch(sensors_delay){
    case FAST:
      sensors_delay = MEDIUM;
      break;

    case MEDIUM:
      sensors_delay = SLOW;
      break;

    case SLOW:
      sensors_delay = VERY_SLOW;
      break;

    case VERY_SLOW:
      sensors_delay = TAKE_A_BREAK;
      break;

    case TAKE_A_BREAK:
      sensors_delay = FAST;
      break;

    default: break;
  };

  Serial.print("New delay: ");
  Serial.println(sensors_delay);
}

void updateStruct(){
  
  Wire.requestFrom(DUE_ADDR, sizeof(sensors_data_t));
  while(!Wire.available())
    delay(100);
  for(uint8_t i=0; Wire.available(); i++)
    sent_data[i] = Wire.read();
}

void getTemperature() {
  // char *temp = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.temperature, "%.02f", sensors_data->temperature);
  Serial.println("Sending temperature " + (String(sensors_data_str.temperature)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.temperature);
  // free(temp);
}

void getHumidity() {
  // char *hum = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.humidity, "%.02f", sensors_data->humidity);
  Serial.println("Sending humidity " + (String(sensors_data_str.humidity)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.humidity);
  // free(hum);
}

void getPressure() {
  // char *pres = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.pressure, "%.02f", sensors_data->pressure);
  Serial.println("Sending pressure " + (String(sensors_data_str.pressure)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.pressure);
  // free(pres);
}

void getAltitude() {
  // char *alt = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.altitude, "%.02f", sensors_data->altitude);
  Serial.println("Sending altitude " + (String(sensors_data_str.altitude)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.altitude);
  // free(alt);
}

void getBrightness() {
  // char *brightn = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.brightness, "%.02f", sensors_data->brightness);
  Serial.println("Sending brightness " + (String(sensors_data_str.brightness)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.brightness);
  // free(brightn);
}

void getDelay(){
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
  Serial.println("Sending delay " + (String(current_delay)) + ".");
  server.send_P(200, "text/plain", current_delay);
  
  free(current_delay);
}

void setDelay(){
  if(server.arg("delay")=="")
      Serial.println("ERROR: For some reason the set-new-delay request has 0 arguments.");
  else{
    String delay_ = server.arg("delay");
    Serial.println("Received new delay request with argument: " + delay_);
      
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
        /*bool success = i2c_send_new_delay(&new_delay.uint8);

        if(success)
          sprintf(result, "ok");
        else
          sprintf(result, "no");
        */
        sprintf(result, "ok");
      }
      else
        sprintf(result, "no");
    
      Serial.print("Sending back response \"");
      Serial.print(result);
      Serial.println("\" to client.");
    }
  
    server.send_P(200, "text/plain", result);
    free(result);
  }
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

bool i2c_send_new_delay(uint8_t *uint8_delay){
  
  Wire.beginTransmission(DUE_ADDR);
  Wire.write(uint8_delay[0]);
  Wire.write(uint8_delay[1]);
  Wire.endTransmission();
  
  Wire.requestFrom(DUE_ADDR, 3);
  while(!Wire.available());
  char *answer = (char *)malloc(sizeof(char) * 3);

  for(uint8_t i=0; Wire.available(); i++)
    answer[i] = Wire.read();

  if(strcmp(answer, "ok\0") == 0){
    old_sensors_delay = sensors_delay;
    return true;
  }
  else{
    sensors_delay = old_sensors_delay;
    return false;
  }
}
