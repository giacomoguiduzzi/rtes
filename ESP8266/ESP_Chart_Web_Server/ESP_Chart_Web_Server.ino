/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <ESP8266mDNS.h>
#include <SPI.h>

const uint8_t SSPIN = 15;

typedef struct {
  float temperature;
  float pressure;
  float humidity;
  float north_direction;
} sensor_data_t;

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

sensor_data_t *sensor_data;
uint8_t *sent_data;
bool struct_reading = false;

SPISettings spisettings;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200, SERIAL_8E1);
  Serial1.begin(115200, SERIAL_8E1);

  sensor_data = (sensor_data_t *)malloc(sizeof(sensor_data));
  sensor_data->temperature = 0.0;
  sensor_data->humidity = 0.0;
  sensor_data->pressure = 0.0;
  sensor_data->north_direction = 0.0;

  sent_data = (uint8_t *)sensor_data;

  // Initialization of SPI library, setting transmitting rate as 1/4 of chip clock (80 MHz / 4 = 20 MHz)
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  spisettings = SPISettings(20000000, MSBFIRST, SPI_MODE0);

  // Setting SS Pin to HIGH to ignore comunication
  pinMode(SSPIN, OUTPUT);
  digitalWrite(SSPIN, HIGH);

  // Initialization of SPIFFS to read files from FLASH memory
  if (!SPIFFS.begin()) {
    Serial1.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Initialization of Wi-Fi library and connection to the network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial1.println("Connecting to WiFi..");
  }

  Serial1.print("IP Address: ");
  Serial1.println(WiFi.localIP());

  // Initialization of the DNS library and set-up of the host name
  if (MDNS.begin("weatherstation"))
    Serial1.println("DNS okay! You can connect to http://weatherstation.local");
  else
    Serial1.println("There was a problem with the DNS initialization but you can still connect via the IP address.");

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html");
  });
  server.on("/functions.js", HTTP_GET, [](AsyncWebServerRequest *request){
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

  server.on("/north", HTTP_GET, [](AsyncWebServerRequest * request) {
    char *n_dir = getNorth();
    request->send_P(200, "text/plain", n_dir);
    free(n_dir);
  });

  server.on("/delay", HTTP_GET, [](AsyncWebServerRequest * request) {
    if(request->hasParam("delay")){
      AsyncWebParameter *p = request->getParam("delay");
      Serial1.print("Received new delay request with argument: ");
      Serial1.println(p->value());
      char * ok = setDelay(p->value());
      request->send_P(200, "text/plain", ok);
      free(ok);
    }
  });

  // Start server
  server.begin();
}

void loop() {
  readSensorData();
  MDNS.update();
  delay(sensors_delay);
}

char *getTemperature() {
  char *temp = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  //while(!digitalRead(SSPIN));
  while(struct_reading);
  sprintf(temp, "%.02f", sensor_data->temperature);
  // Serial.print("Sending temperature value to client: ");
  // Serial.println(String(temp));
  return temp;
}

char *getHumidity() {
  char *hum = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(struct_reading);
  sprintf(hum, "%.02f", sensor_data->humidity);
  // Serial.print("Sending humidity value to client: ");
  // Serial.println(String(hum));
  return hum;
}

char *getPressure() {
  char *pres = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(struct_reading);
  sprintf(pres, "%.02f", sensor_data->pressure);
  // Serial.print("Sending pressure value to client: ");
  //Serial.println(String(pres));
  return pres;
}

char *getNorth() {
  char *n_dir = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(struct_reading);
  sprintf(n_dir, "%.02f", sensor_data->north_direction);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return n_dir;
}

char *setDelay(String delay_) {
  char *result = (char *)malloc(sizeof(char) * 3); 
  uint16_t new_delay = strtol(delay_.c_str(), NULL, DEC);

  switch(new_delay){
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
      Serial1.print("Received delay is not an accepted value: ");
      Serial1.println(new_delay);
      sprintf(result, "no");
      return result;
    break;
  }

  Serial1.print("New delay in setDelay(): ");
  Serial1.println(new_delay);

  sendNewDelay(new_delay);

  // check if the SPI channel is not being used
  // (if digitalRead returns 0 the pin is LOW so it's being used
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  // call function to update delay
  // readSensorData();
  
  sprintf(result, "ok");

  Serial1.print("Sending back response \"");
  Serial1.print(result);
  Serial1.println("\" to client.");

  return result;
}

void sendNewDelay(uint16_t new_delay){
  uint8_t *uint8_new_delay = (uint8_t *)&new_delay;

  SPI.beginTransaction(spisettings);
  // Enable Slave comunication
  digitalWrite(SSPIN, LOW);

  SPI.transfer(uint8_new_delay[0]);
  SPI.transfer(uint8_new_delay[1]);

  // Disable Slave comunication
  digitalWrite(SSPIN, HIGH);
  SPI.endTransaction();
}

void readSensorData() {
  // char message[100];
  // uint8_t *uint8_new_delay = (uint8_t *)&new_delay;
  // uint8_t new_delay_counter = 0;

  if(Serial.available() > 0){
    struct_reading = true;
    for(uint8_t i=0; i < sizeof(sensor_data); i++)
      sent_data[i] = Serial.read();

    struct_reading = false;

    Serial1.print("Temperature: ");
    Serial1.print(sensor_data->temperature);
    Serial1.println("°C");

    Serial1.print("(HEX: ");
    Serial1.print(sent_data[0], HEX);
    Serial1.print(sent_data[1], HEX);
    Serial1.print(sent_data[2], HEX);
    Serial1.print(sent_data[3], HEX);
    Serial1.println(")");

    Serial1.print("Pressure: ");
    Serial1.print(sensor_data->humidity);
    Serial1.println(" hPa");

    Serial1.print("Humidity: ");
    Serial1.print(sensor_data->humidity);
    Serial1.println("%");

    Serial1.print("North direction: ");
    Serial1.print(sensor_data->north_direction);
    Serial1.println("°");
    Serial1.println("");
  }
  // Serial.println("Reading data structure: ");
  // SPI.beginTransaction(spisettings);
  // Enable Slave comunication
  // digitalWrite(SSPIN, LOW);

  // for (uint8_t i = 0; i < sizeof(sensor_data); i++) {
  //   if(new_delay_counter < 2 && new_delay != 0){
  //     sent_data[i] = SPI.transfer(uint8_new_delay[new_delay_counter]);
  //     new_delay_counter++;
  //   }
  //   else
  //     sent_data[i] = SPI.transfer(0x00);
  //  }

  // Disable Slave comunication
  // digitalWrite(SSPIN, HIGH);
  // SPI.endTransaction();

  // sensor_data = (sensor_data_t *) sent_data;
  // Serial.println(snprintf(message, sizeof(message), "Temperature: %f °C", sensor_data->temperature));
  // Serial.print("Temperature: ");
  // Serial.print(sensor_data->temperature);
  // Serial.println("°C");

  // Serial.print("(HEX: ");
  // Serial.print(sent_data[0], HEX);
  // Serial.print(sent_data[1], HEX);
  // Serial.print(sent_data[2], HEX);
  // Serial.print(sent_data[3], HEX);
  // Serial.println(")");

  // Serial.print("Pressure: ");
  // Serial.print(sensor_data->humidity);
  // Serial.println(" hPa");

  // Serial.print("Humidity: ");
  // Serial.print(sensor_data->humidity);
  // Serial.println("%");

  // Serial.print("North direction: ");
  // Serial.print(sensor_data->north_direction);
  // Serial.println("°");
  // Serial.println("");
  // Serial.println(snprintf(message, sizeof(message), "Pressure: %f hPa", sensor_data->pressure));
  // Serial.println(snprintf(message, sizeof(message), "Humidity: %f\%", sensor_data->humidity));
  // Serial.println(snprintf(message, sizeof(message), "North direction: %f°", sensor_data->north_direction));
}
