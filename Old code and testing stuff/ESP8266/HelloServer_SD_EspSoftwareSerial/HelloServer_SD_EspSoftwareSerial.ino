#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <SD.h>
#include <SoftwareSerial.h>

#ifndef STASSID
#define STASSID "Vodafone - Packets Are Coming"
#define STAPSK  "Arouteroficeandfire96!"
#endif

// Pin that blinks during data transmission on UART
#define WORKING_LED_PIN 4

const char* ssid = STASSID;
const char* password = STAPSK;

// data struct from Arduino
typedef struct {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float brightness;
} sensors_data_t;

// data struct as char arrays to optimize sending data to client
typedef struct{
  char temperature[5];
  char humidity[5];
  char pressure[5];
  char altitude[5];
  char brightness[5];
} sensors_data_str_t;

// delay struct
typedef enum
{
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

uint16_t sensors_delay = MEDIUM;
uint16_t old_sensors_delay = MEDIUM;

sensors_data_t sensors_data;
sensors_data_str_t sensors_data_str;
uint8_t *sent_data;

// webserver
ESP8266WebServer server(80);

// booleans to change measure unit
bool celsius = true, pascal = true;

// Software Serial to read from Arduino
SoftwareSerial SWSerial;

// array in which the Arduino answer for a new delay from client is stored
char delay_answer[3];

// Function that handles the '/' directory requested from client, answering with the index.html file
void handleRoot() {
  Serial.println("Received request for index page.");
  
  File index_file = SD.open("/index.html");
  // stream file from SD to client
  size_t sent = server.streamFile(index_file, "text/html");
  // check for possible problems
  if(sent != index_file.size())
    Serial.println("Sent less bytes to client than expected for the index page.");
  
  index_file.close();
  
  Serial.println("Page index.html sent.");
}

// Function that answers to the client with the JS file
void handleJS(){
  Serial.println("Received request for JS file.");
  
  File functions_file = SD.open("/functions.js");

  size_t sent = server.streamFile(functions_file, "text/javascript");

  if(sent != functions_file.size())
    Serial.println("Sent less bytes to client than expected for the functions.js file");

  functions_file.close();

  Serial.println("JS sent.");
}

// Function that manages not valid URLs
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
  // pins setup
  pinMode(LED_BUILTIN, OUTPUT);
  // set built-in led to high that shuts down the led, inverted logic
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(WORKING_LED_PIN, OUTPUT);
  digitalWrite(WORKING_LED_PIN, LOW);
  
  Serial.begin(115200);
  while(!Serial)
    delay(100);

  // Software Serial init, TX pin is -1 because it's not used
  SWSerial.begin(115200, SWSERIAL_8N1, 5, -1); // RX, TX not used

  // Serial1 only transmits but it's a Hardware Serial, more reliable and efficient
  Serial1.begin(115200); // TX only on pin GPIO2 (D4)
  while(!Serial1)
    delay(100);

  // Initialization of SD card reader 
  if (!SD.begin(15)) {
    Serial.println("Couldn't initialize the SD Reader. Board stalling.");
    while(1)
      delay(1000);
  }

  // check for file presence
  if(!SD.exists("/index.html") || !SD.exists("/functions.js")){
    Serial.println("It looks like some files are missing on the SD Card. Board stalling.");
    while(1)
      delay(1000);
  }

  Serial.println("SD initialized.");

  // Init of data struct
  sensors_data.temperature = 0;
  sensors_data.humidity = 0;
  sensors_data.pressure = 0;
  sensors_data.altitude = 0;
  sensors_data.brightness = 0;

  sent_data = (uint8_t *)&sensors_data;

  // Setup of WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

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

  // Setting up association of pages to the webserver's URLs
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

  // Start webserver
  server.begin();
  Serial.println("HTTP server started.");
}

void loop(void) {
  // manage client requests
  server.handleClient();

  // if Arduino sent some data
  if(SWSerial.available()){
    // turn on green LED
    digitalWrite(WORKING_LED_PIN, HIGH);
    Serial.println("SWSerial new data.");
    // get the first char in the buffer
    char peeked = (char)SWSerial.peek();
    Serial.print("First char: ");
    Serial.println(peeked);

    // in this case it's an answer from Arduino that tells us if the new delay we just sent was sent correctly or not
    if(peeked == 'd'){
      // ignore 'd' letter
      SWSerial.read();
      
      // read answer, 3 bytes
      for(uint8_t i=0; i<3; i++)
        delay_answer[i] = (char)SWSerial.read();

      // malloc new array to answer web client
      char *result = (char *)malloc(sizeof(char) * 3);

      // if Arduino sent 'ok' answer 'ok' to the client
      if(strcmp(delay_answer, "ok\0") == 0){
        Serial.println("Arduino new delay ok.");
        old_sensors_delay = sensors_delay;
        sprintf(result, "ok");
        Serial.println("new delay \"ok\" to client.");
      }

      // else answer 'no' to the client
      else if(strcmp(delay_answer, "no\0") == 0){
        Serial.println("Arduino bad new delay.");
        sensors_delay = old_sensors_delay;
        sprintf(result, "no");
        Serial.println("new delay \"no\" to client.");
      }

      else{
        Serial.println("Arduino bad answer.");
        sprintf(result, "no");
        Serial.println("new delay \"no\" to client.");
      }

      // send answer to the client
      server.send_P(200, "text/plain", result);

      // free memory
      free(result);
    }
    // else if the first char is 'n', the user pressed the physical button and Arduino sent us a new delay
    else if(peeked == 'n'){
      // skip first byte 'n'
      SWSerial.read();

      Serial.print("New delay from button: ");

      // prepare data struct for the new delay
      union{
        uint16_t uint16;
        uint8_t uint8[2];
      } new_delay;

      // read 2 bytes
      new_delay.uint8[0] = SWSerial.read();
      new_delay.uint8[1] = SWSerial.read();

      Serial.println(new_delay.uint16);

      // try to the set the new delay
      bool ok = set_new_delay(new_delay.uint16);
      // write 'n' to let the Arduino know we're talking about the delay it just sent
      Serial1.write('n');
      // if ok send 1, else 0
      if(ok)
        Serial1.write(0x01);
      else
        Serial1.write(0x00);
    }
    // if the first byte is neither 'n' or 'd' then it's the new sensors data struct
    else{
      Serial.println("New Data: ");
      // read sizeof(sensors_data_t) bytes into the data struct
      for(uint8_t i=0; i<sizeof(sensors_data_t); i++)
        sent_data[i] = SWSerial.read();
      
      Serial_print_data_struct();
    }
    // turn off green LED
    digitalWrite(WORKING_LED_PIN, LOW);
  }

  // sleep a bit to let the hardware do its stuff
  delay(50);
}

// Function to print sensors data struct
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

// Answer the client with the new temperature
void getTemperature() {
  if(celsius)
    sprintf(sensors_data_str.temperature, "%.02f", sensors_data.temperature);
  else
    // fahrenheit
    sprintf(sensors_data_str.temperature, "%.02f", (sensors_data.temperature * 1.8) + 32);
    
  server.send_P(200, "text/plain", sensors_data_str.temperature);
}

// Answer the client with the new humidity
void getHumidity() {
  sprintf(sensors_data_str.humidity, "%.02f", sensors_data.humidity);
  server.send_P(200, "text/plain", sensors_data_str.humidity);
}

// Answer the client with the new pressure
void getPressure() {
  if(pascal)
    sprintf(sensors_data_str.pressure, "%.02f", sensors_data.pressure);
  else
    // bar
    sprintf(sensors_data_str.pressure, "%.02f", sensors_data.pressure / 100000);
  server.send_P(200, "text/plain", sensors_data_str.pressure);
}

// Answer the client with the new altitude
void getAltitude() {
  sprintf(sensors_data_str.altitude, "%.02f", sensors_data.altitude);
  server.send_P(200, "text/plain", sensors_data_str.altitude);
}

// Answer the client with the new brightness
void getBrightness() {
  sprintf(sensors_data_str.brightness, "%.02f", sensors_data.brightness);
  server.send_P(200, "text/plain", sensors_data_str.brightness);
}

// Answer the client with the new delay
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

// set the new delay sent from the web client and send it to the Arduino too
void setDelay(){
  // if there's no arguments something wrong is going on
  if(server.arg("delay")=="")
      Serial.println("ERROR: For some reason the set-new-delay request has 0 arguments.");
  // get the delay argument, evaluate it and try to set it
  else{
    String delay_ = server.arg("delay");
    Serial.println("Received new delay request with argument: " + delay_);
      
    union{
      uint16_t uint16;
      uint8_t uint8[2];
    } new_delay;

    // conversion from char array to unsigned long, but the number can fit in an uint16_t
    new_delay.uint16 = strtol(delay_.c_str(), NULL, DEC);
  
    Serial.print("New delay in setDelay(): ");
    Serial.println(new_delay.uint16);

    // if the delay is the same don't do anything, just answer okay
    if(new_delay.uint16 == sensors_delay){
      char * result = (char *)malloc(sizeof(char) * 3);
      sprintf(result, "ok");
      Serial.print("Sending back response \"");
      Serial.print(result);
      Serial.println("\" to client.");
      server.send_P(200, "text/plain", result);
      free(result);
    }
    // else try to set the new delay and send it to Arduino
    else{
      old_sensors_delay = sensors_delay;
      bool answer = set_new_delay(new_delay.uint16);

      // if it's okay, send to Arduino but don't answer the web client as we need its answer first. 
      // The answer will be given in the loop() function
      if (answer) {
        Serial1.write(new_delay.uint8[0]);
        Serial1.write(new_delay.uint8[1]);
      }
      // not okay, send 'no' to client
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

// function to try to set the new delay
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

// function that sets measure units checking its values
void setUnits(){
  // if the arguments are empty something is wrong
  if(server.arg("temp")=="" && server.arg("press")=="")
      Serial.println("ERROR: For some reason the setUnits request has lesser than 2 arguments.");
  // else get the arguments, evaluate them and set up boolean values accordingly
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
