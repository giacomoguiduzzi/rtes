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
#include <SoftwareSerial.h>
// #include <SPI.h>

// const uint8_t SSPIN = 15;
#define DELAY_FLAG 0x64 // "d" letter

typedef struct {
  float temperature;
  float pressure;
  float humidity;
  float altitude;
  float brightness;
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
bool updating_struct, sending_delay;

// SPISettings spisettings;
SoftwareSerial uart_data;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

unsigned long start_time, end_time, loop_delay;

void setup() {
  // Serial port for debugging purposes (USB Cable)
  Serial.begin(115200);
  uart_data.begin(38400, SWSERIAL_8N1, 13, 15, false); // RXPin, TXPin. For some reason the function for ESP8266 is different and needs to be initialized like this
  // Serial.begin(115200, SERIAL_8E1);

  updating_struct = sending_delay = false;

  sensor_data = (sensor_data_t *)malloc(sizeof(sensor_data));
  sensor_data->temperature = 0.0;
  sensor_data->humidity = 0.0;
  sensor_data->pressure = 0.0;
  sensor_data->altitude = 0.0;
  sensor_data->brightness = 0.0;

  sent_data = (uint8_t *)sensor_data;

  // Initialization of SPI library, setting transmitting rate as 1/4 of chip clock (80 MHz / 4 = 20 MHz)
  // SPI.begin();
  // SPI.setClockDivider(SPI_CLOCK_DIV4);
  // spisettings = SPISettings(20000000, MSBFIRST, SPI_MODE0);

  // Setting SS Pin to HIGH to ignore comunication
  // pinMode(SSPIN, OUTPUT);
  // digitalWrite(SSPIN, HIGH);

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

  start_time = end_time = loop_delay = 0;

  server_setup();
}

void loop() {
  start_time = millis();
  if(!sending_delay)
    readSensorData();
  MDNS.update();
  end_time = millis();

  loop_delay = sensors_delay - (end_time - start_time);

  if(loop_delay > 0)
    delay(loop_delay);
}

void server_setup(){
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
    if(request->hasParam("delay")){
      AsyncWebParameter *p = request->getParam("delay");
      Serial.print("Received new delay request with argument: ");
      Serial.println(p->value());
      char * ok = setDelay(p->value());
      request->send_P(200, "text/plain", ok);
      free(ok);
    }
  });

  // Start server
  server.begin();  
}

char *getTemperature() {
  char *temp = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  //while(!digitalRead(SSPIN));
  while(updating_struct);
  sprintf(temp, "%.02f", sensor_data->temperature);
  // Serial.print("Sending temperature value to client: ");
  // Serial.println(String(temp));
  return temp;
}

char *getHumidity() {
  char *hum = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(updating_struct);
  sprintf(hum, "%.02f", sensor_data->humidity);
  // Serial.print("Sending humidity value to client: ");
  // Serial.println(String(hum));
  return hum;
}

char *getPressure() {
  char *pres = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(updating_struct);
  sprintf(pres, "%.02f", sensor_data->pressure);
  // Serial.print("Sending pressure value to client: ");
  //Serial.println(String(pres));
  return pres;
}

char *getAltitude() {
  char *alt = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(updating_struct);
  sprintf(alt, "%.02f", sensor_data->altitude);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return alt;
}

char *getBrightness() {
  char *brightn = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while(updating_struct);
  sprintf(brightn, "%.02f", sensor_data->brightness);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return brightn;
}

char *setDelay(String delay_) {
  char *result = (char *)malloc(sizeof(char) * 3); 
  uint16_t new_delay = strtol(delay_.c_str(), NULL, DEC);

  Serial.print("New delay in setDelay(): ");
  Serial.println(new_delay);

  bool answer = sendNewDelay(new_delay);

  // check if the SPI channel is not being used
  // (if digitalRead returns 0 the pin is LOW so it's being used
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  // call function to update delay
  // readSensorData();
  if(answer){
    sprintf(result, "ok");
  }
  else
    sprintf(result, "no");

  Serial.print("Sending back response \"");
  Serial.print(result);
  Serial.println("\" to client.");

  return result;
}

bool sendNewDelay(uint16_t new_delay){
  uint8_t *uint8_new_delay = (uint8_t *)&new_delay;
  uint8_t flag;
  bool received_answer = false, result = false;

  // Waiting for resource to free up
  while(updating_struct)
    delay(100);

  sending_delay = true;
  uart_data.write(uint8_new_delay[0]);
  uart_data.write(uint8_new_delay[1]);

  while(!received_answer){
    if(uart_data.available()){
        flag = uart_data.read();

        if(flag == DELAY_FLAG){
          received_answer = true;
          uint8_new_delay[0] = uart_data.read();
          uint8_new_delay[1] = uart_data.read();

          result = set_new_delay(new_delay);
        }
    }
  }

  sending_delay = false;

  return result;
  // SPI.beginTransaction(spisettings);
  // Enable Slave comunication
  // digitalWrite(SSPIN, LOW);

  // SPI.transfer(uint8_new_delay[0]);
  // SPI.transfer(uint8_new_delay[1]);

  // Disable Slave comunication
  // digitalWrite(SSPIN, HIGH);
  // SPI.endTransaction();
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

void readSensorData() {
  // char message[100];
  // uint8_t *uint8_new_delay = (uint8_t *)&new_delay;
  // uint8_t new_delay_counter = 0;
  
  if(uart_data.available()){
    updating_struct = true;
    for(uint8_t i=0; i < sizeof(sensor_data); i++)
      sent_data[i] = uart_data.read();

    updating_struct = false;

    Serial.print("Temperature: ");
    Serial.print(sensor_data->temperature);
    Serial.println("°C");

    Serial.print("(HEX: ");
    Serial.print(sent_data[0], HEX);
    Serial.print(sent_data[1], HEX);
    Serial.print(sent_data[2], HEX);
    Serial.print(sent_data[3], HEX);
    Serial.println(")");

    Serial.print("Pressure: ");
    Serial.print(sensor_data->pressure);
    Serial.println(" hPa");

    Serial.print("Humidity: ");
    Serial.print(sensor_data->humidity);
    Serial.println("%");

    Serial.print("Altitude: ");
    Serial.print(sensor_data->altitude);
    Serial.println("m");

    Serial.print("Brightness: ");
    Serial.print(sensor_data->brightness);
    Serial.println(" lux");
    Serial.println("");
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
