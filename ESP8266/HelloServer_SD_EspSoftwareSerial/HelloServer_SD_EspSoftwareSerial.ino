#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
// #include <ESP8266mDNS.h>
#include <SD.h>
#include <SoftwareSerial.h>

#ifndef STASSID
#define STASSID "Vodafone - Packets Are Coming"
#define STAPSK  "Arouteroficeandfire96!"
#endif

// #define DUE_ADDR 0x33
#define WORKING_LED_PIN 4

const char* ssid = STASSID;
const char* password = STAPSK;

typedef struct {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float brightness;
} sensors_data_t;

typedef struct{
  char temperature[5];
  char humidity[5];
  char pressure[5];
  char altitude[5];
  char brightness[5];
} sensors_data_str_t;

typedef enum
{
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

uint16_t sensors_delay = MEDIUM;
uint16_t old_sensors_delay = MEDIUM;

volatile bool sending_pages = false;

sensors_data_t sensors_data;
sensors_data_str_t sensors_data_str;
uint8_t *sent_data;

ESP8266WebServer server(80);

unsigned long start_time, end_time, loop_delay;

bool celsius = true, pascal = true;

// uint16_t mDNS_update_counter = 0;

SoftwareSerial SWSerial;

char delay_answer[3];

void handleRoot() {
  sending_pages = true;
  // server.send(200, "text/plain", "hello from esp8266!\r\n");
  Serial.println("Received request for index page.");
  
  File index_file = SD.open("/index.html");
  
  size_t sent = server.streamFile(index_file, "text/html");
  
  if(sent != index_file.size())
    Serial.println("Sent less bytes to client than expected for the index page.");
  
  index_file.close();

  Serial.println("Page index.html sent.");
  sending_pages = false;
}

void handleJS(){
  Serial.println("Received request for JS file.");
  
  File functions_file = SD.open("/functions.js");

  size_t sent = server.streamFile(functions_file, "text/javascript");

  if(sent != functions_file.size())
    Serial.println("Sent less bytes to client than expected for the functions.js file");

  functions_file.close();

  Serial.println("JS sent.");
  sending_pages = false;
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
  // ISR setup
  // pinMode(ISR_BUTTON_PIN, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(ISR_BUTTON_PIN), buttonDelay, RISING);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(WORKING_LED_PIN, OUTPUT);
  digitalWrite(WORKING_LED_PIN, LOW);
  
  Serial.begin(115200);
  while(!Serial)
    delay(100);

  SWSerial.begin(115200, SWSERIAL_8N1, 5, -1); // RX, TX not used
  
  Serial1.begin(115200); // TX only on pin GPIO2 (D4)
  while(!Serial1)
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

  // sensors_data = (sensors_data_t *)malloc(sizeof(sensors_data_t));
  sensors_data.temperature = 0;
  sensors_data.humidity = 0;
  sensors_data.pressure = 0;
  sensors_data.altitude = 0;
  sensors_data.brightness = 0;

  sent_data = (uint8_t *)&sensors_data;

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

  /* if (MDNS.begin("weatherstation")) {
    Serial.println("MDNS responder started");
  } */

  server.on("/", handleRoot);
  server.on("/functions.js", handleJS);

  server.on("/temperature", getTemperature);
  server.on("/humidity", getHumidity);
  server.on("/pressure", getPressure);
  server.on("/altitude", getAltitude);
  server.on("/brightness", getBrightness);
  server.on("/delay", setDelay);
  server.on("/getdelay", getDelay);
  server.on("/units", setUnits);

  server.onNotFound(handleNotFound);


  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  /*if(mDNS_update_counter >= 3000){
      Serial.println("Updating mDNS.");
      MDNS.update();
      mDNS_update_counter = 0;
    }
    else
      mDNS_update_counter++; */
  
  server.handleClient();

  if(SWSerial.available()){
    digitalWrite(WORKING_LED_PIN, HIGH);
    Serial.println("SWSerial new data.");
    char peeked = (char)SWSerial.peek();
    Serial.print("First char: ");
    Serial.println(peeked);
    
    if(peeked == 'd'){
      SWSerial.read();
      
      for(uint8_t i=0; i<3; i++)
        delay_answer[i] = (char)SWSerial.read();

      char *result = (char *)malloc(sizeof(char) * 3);
      
      if(strcmp(delay_answer, "ok\0") == 0){
        Serial.println("Arduino new delay ok.");
        old_sensors_delay = sensors_delay;
        sprintf(result, "ok");
        Serial.println("new delay \"ok\" to client.");
        server.send_P(200, "text/plain", result);
      }
      
      else if(strcmp(delay_answer, "no\0") == 0){
        Serial.println("Arduino bad new delay.");
        sensors_delay = old_sensors_delay;
        sprintf(result, "no");
        Serial.println("new delay \"no\" to client.");
        server.send_P(200, "text/plain", result);
      }

      else{
        Serial.println("Arduino bad answer.");
        sprintf(result, "no");
        Serial.println("new delay \"no\" to client.");
        server.send_P(200, "text/plain", result);
      }
      
      free(result);
    }
    else if(peeked == 'n'){
      SWSerial.read();

      Serial.print("New delay from button: ");
      
      union{
        uint16_t uint16;
        uint8_t uint8[2];
      } new_delay;
      
      new_delay.uint8[0] = SWSerial.read();
      new_delay.uint8[1] = SWSerial.read();

      Serial.println(new_delay.uint16);

      bool ok = set_new_delay(new_delay.uint16);
      Serial1.write('n');
      if(ok)
        Serial1.write(0x01);
      else
        Serial1.write(0x00);
    }

    else{
      Serial.println("New Data: ");
      for(uint8_t i=0; i<sizeof(sensors_data_t); i++)
        sent_data[i] = SWSerial.read();
      
      Serial_print_data_struct();
      
    }

    digitalWrite(WORKING_LED_PIN, LOW);
  }

  delay(50);

  /* int loop_delay = sensors_delay - (end_time - start_time);

  if(loop_delay > 0)
    delay(loop_delay);*/
}

/* void updateStruct(){
  
  Wire.requestFrom(DUE_ADDR, sizeof(sensors_data_t));
  while(!Wire.available())
    delay(100);
  for(uint8_t i=0; Wire.available(); i++)
    sent_data[i] = Wire.read();
} */

void Serial_print_data_struct(){
  
  Serial.print("T:");
  Serial.print(sensors_data.temperature);
  Serial.print("/");

  Serial.print("H:");
  Serial.print(sensors_data.humidity);
  Serial.print("/");

  Serial.print("P:");
  Serial.print(sensors_data.pressure);
  Serial.print("/");

  Serial.print("A:");
  Serial.print(sensors_data.altitude);
  Serial.print("/");

  Serial.print("B");
  Serial.print(sensors_data.brightness);
  Serial.println("/");
}

void getTemperature() {
  // char *temp = (char *)malloc(sizeof(char) * 5);
  if(celsius)
    sprintf(sensors_data_str.temperature, "%.02f", sensors_data.temperature);
  else
    // fahrenheit
    sprintf(sensors_data_str.temperature, "%.02f", (sensors_data.temperature * 1.8) + 32);
    
  //Serial.println("Sending temperature " + (String(sensors_data_str.temperature)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.temperature);
  // free(temp);
}

void getHumidity() {
  // char *hum = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.humidity, "%.02f", sensors_data.humidity);
  //Serial.println("Sending humidity " + (String(sensors_data_str.humidity)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.humidity);
  // free(hum);
}

void getPressure() {
  // char *pres = (char *)malloc(sizeof(char) * 5);
  if(pascal)
    sprintf(sensors_data_str.pressure, "%.02f", sensors_data.pressure);
  else
    // bar
    sprintf(sensors_data_str.pressure, "%.02f", sensors_data.pressure / 100000);
  //Serial.println("Sending pressure " + (String(sensors_data_str.pressure)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.pressure);
  // free(pres);
}

void getAltitude() {
  // char *alt = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.altitude, "%.02f", sensors_data.altitude);
  //Serial.println("Sending altitude " + (String(sensors_data_str.altitude)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.altitude);
  // free(alt);
}

void getBrightness() {
  // char *brightn = (char *)malloc(sizeof(char) * 5);
  sprintf(sensors_data_str.brightness, "%.02f", sensors_data.brightness);
  //Serial.println("Sending brightness " + (String(sensors_data_str.brightness)) + ".");
  server.send_P(200, "text/plain", sensors_data_str.brightness);
  // free(brightn);
}

void getDelay(){
  char *current_delay;

  switch(sensors_delay){
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
      
    union{
      uint16_t uint16;
      uint8_t uint8[2];
    } new_delay;
  
    new_delay.uint16 = strtol(delay_.c_str(), NULL, DEC);
  
    Serial.print("New delay in setDelay(): ");
    Serial.println(new_delay.uint16);
  
    if(new_delay.uint16 == sensors_delay){
      char * result = (char *)malloc(sizeof(char) * 3);
      sprintf(result, "ok");
      Serial.print("Sending back response \"");
      Serial.print(result);
      Serial.println("\" to client.");
      server.send_P(200, "text/plain", result);
      free(result);
    }
      
    else
    {
      old_sensors_delay = sensors_delay;
      bool answer = set_new_delay(new_delay.uint16);
    
      if (answer) {
        Serial1.write(new_delay.uint8[0]);
        Serial1.write(new_delay.uint8[1]);
      }
      else{
        char * result = (char *)malloc(sizeof(char) * 3);
        sprintf(result, "no");
        Serial.print("Sending back response \"");
        Serial.print(result);
        Serial.println("\" to client.");
        server.send_P(200, "text/plain", result);
        free(result);
      }
    }
  }
}

bool set_new_delay(const uint16_t new_delay) {
  bool ok = true;

  switch (new_delay) {
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

/* bool i2c_send_new_delay(uint8_t *uint8_delay){
  
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
} */

void setUnits(){
  if(server.arg("temp")=="" && server.arg("press")=="")
      Serial.println("ERROR: For some reason the setUnits request has lesser than 2 arguments.");
  else{
    String temp = server.arg("temp");
    String press = server.arg("press");
    Serial.println("Received new units request with arguments: temp = " + temp + ", press = " + press + ".");
      
    char *result = (char *)malloc(sizeof(char) * 3);

    if(strcmp(temp.c_str(), "Celsius\0") == 0)
      celsius = true;
    else
      celsius = false;

    if(strcmp(press.c_str(), "Pascal\0") == 0)
      pascal = true;
    else
      pascal = false;
  
    sprintf(result, "ok");

    Serial.println("Set up new units. Sending response " + (String(result)) + " to client.");
  
    server.send_P(200, "text/plain", result);
    free(result);
  }
}
