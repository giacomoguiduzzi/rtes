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
#include <FS.h>
#include <ESP8266mDNS.h>
// #include <SPI.h>

#define DELAY_FLAG 0x64 // "d" letter

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

// SPISettings spisettings;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

unsigned long start_time, end_time, loop_delay;

void setup() {
  // Serial port for debugging purposes (USB Cable)
  Serial.begin(115200);
  Wire.begin(0x4A);
  Wire.onReceive(readSensorData);
  Wire.onRequest(sendNewDelay);

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

  // Initialization of SPI library, setting transmitting rate as 1/4 of chip clock (80 MHz / 4 = 20 MHz)
  // SPI.begin();
  // SPI.setClockDivider(SPI_CLOCK_DIV4);
  // spisettings = SPISettings(20000000, MSBFIRST, SPI_MODE0);

  // Setting SS Pin to HIGH to ignore comunication
  // pinMode(SSPIN, OUTPUT);
  // digitalWrite(SSPIN, HIGH);

  Serial.println("Setting up SPIFFS.");

  // Initialization of SPIFFS to read files from FLASH memory
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.print("Connecting to WiFi..");
  // Initialization of Wi-Fi library and connection to the network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
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

  start_time = end_time = loop_delay = 0;

  Serial.println("Setting up server pages.");
  server_setup();

  Serial.println("Setup complete.");
}

void loop() {
  start_time = millis();
  MDNS.update();
  end_time = millis();

  loop_delay = sensors_delay - (end_time - start_time);

  if (loop_delay > 0)
    delay(loop_delay);
}

void server_setup() {
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html");
  });
  server.on("/functions.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/functions.js");
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

void sendNewDelay() {
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
