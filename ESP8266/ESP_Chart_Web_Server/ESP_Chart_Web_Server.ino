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
// #include <stdio.h>

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
uint16_t new_delay = 0;

sensor_data_t *sensor_data;
uint8_t *sent_data = (uint8_t *)malloc(sizeof(sensor_data));

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  // Serial port for debugging purposes
  Serial.begin(9600);

  // Initialization of SPI library, setting transmitting rate as 1/4 of chip clock (80 MHz / 4 = 20 MHz)
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);

  // Setting SS Pin to HIGH to ignore comunication
  pinMode(SSPIN, OUTPUT);
  digitalWrite(SSPIN, HIGH);

  // Initialization of SPIFFS to read files from FLASH memory
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Initialization of Wi-Fi library and connection to the network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialization of the DNS library and set-up of the host name
  if (MDNS.begin("weatherstation"))
    Serial.println("DNS okay! You can connect to http://weatherstation.local");
  else
    Serial.println("There was a problem with the DNS initialization but you can still connect via the IP address.");

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html");
  });
  server.on("/functions.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/functions.js");
    });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getTemperature());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getHumidity());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getPressure());
  });

  server.on("/north", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getNorth());
  });

  server.on("/delay", HTTP_GET, [](AsyncWebServerRequest * request) {
    if(request->hasParam("delay")){
      AsyncWebParameter *p = request->getParam("delay");
      Serial.print("Received new delay request with argument: ");
      Serial.println(p->value());
      request->send_P(200, "text/plain", setDelay(p->value()));
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
  char temp[5];
  sprintf(temp, "%.02f", sensor_data->temperature);
  Serial.print("Sending temperature value to client: ");
  Serial.println(temp);
  return temp;
}

char *getHumidity() {
  char hum[5];
  sprintf(hum, "%.02f", sensor_data->humidity);
  Serial.print("Sending humidity value to client: ");
  Serial.println(hum);
  return hum;
}

char *getPressure() {
  char pres[5];
  sprintf(pres, "%.02f", sensor_data->pressure);
  Serial.print("Sending pressure value to client: ");
  Serial.println(pres);
  return pres;
}

char *getNorth() {
  char n_dir[5];
  sprintf(n_dir, "%.02f", sensor_data->north_direction);
  Serial.print("Sending north direction value to client: ");
  Serial.println(n_dir);
  return n_dir;
}

char *setDelay(String delay_) {
  char ok[3]; 
  new_delay = strtol(delay_.c_str(), NULL, 10);
  
  Serial.print("New delay in setDelay(): ");
  Serial.println(new_delay);

  // check if the SPI channel is not being used
  // (if digitalRead returns 0 the pin is LOW so it's being used
  /* while (!digitalRead(SSPIN)) {
    delay(250);
  }*/
  // active wait, can't use delay() here
  while(!digitalRead(SSPIN));
  // call function to update delay
  readSensorData();

  sprintf(ok, "ok");

  Serial.print("Sending back response ");
  Serial.print(ok);
  Serial.println(" to client.");

  return ok;
}

void readSensorData() {
  // char message[100];
  uint8_t *uint8_new_delay = (uint8_t *)&new_delay;
  uint8_t new_delay_counter = 0;

  // Serial.println("Reading data structure: ");
  // SPI.beginTransaction(spisettings);
  // Enable Slave comunication
  digitalWrite(SSPIN, LOW);

  for (int i = 0; i < sizeof(sensor_data); i++) {
    if(new_delay_counter < 2 && new_delay != 0){
      sent_data[i] = SPI.transfer(new_delay);
      new_delay_counter++;
    }
    else
      sent_data[i] = SPI.transfer(0x00);
  }

  // Disable Slave comunication
  digitalWrite(SSPIN, HIGH);
  // SPI.endTransaction();

  sensor_data = (sensor_data_t *) sent_data;
  // Serial.println(snprintf(message, sizeof(message), "Temperature: %f °C", sensor_data->temperature));
  /* Serial.print("Temperature: ");
  Serial.print(sensor_data->temperature);
  Serial.println("°C");

  Serial.print("(HEX: ");
  Serial.print(sent_data[0], HEX);
  Serial.print(sent_data[1], HEX);
  Serial.print(sent_data[2], HEX);
  Serial.print(sent_data[3], HEX);
  Serial.println(")");

  Serial.print("Pressure: ");
  Serial.print(sensor_data->humidity);
  Serial.println(" hPa");

  Serial.print("Humidity: ");
  Serial.print(sensor_data->humidity);
  Serial.println("%");

  Serial.print("North direction: ");
  Serial.print(sensor_data->north_direction);
  Serial.println("°");*/
  // Serial.println("");
  // Serial.println(snprintf(message, sizeof(message), "Pressure: %f hPa", sensor_data->pressure));
  // Serial.println(snprintf(message, sizeof(message), "Humidity: %f\%", sensor_data->humidity));
  // Serial.println(snprintf(message, sizeof(message), "North direction: %f°", sensor_data->north_direction));
}
