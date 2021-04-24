/*
 SCP1000 Barometric Pressure Sensor Display

 Shows the output of a Barometric Pressure Sensor on a
 Uses the SPI library. For details on the sensor, see:
 http://www.sparkfun.com/commerce/product_info.php?products_id=8161
 http://www.vti.fi/en/support/obsolete_products/pressure_sensors/

 This sketch adapted from Nathan Seidle's SCP1000 example for PIC:
 http://www.sparkfun.com/datasheets/Sensors/SCP1000-Testing.zip

 Circuit:
 SCP1000 sensor attached to pins 6, 7, 10 - 13:
 DRDY: pin 6
 CSB: pin 7
 MOSI: pin 11
 MISO: pin 12
 SCK: pin 13

 created 31 July 2010
 modified 14 August 2010
 by Tom Igoe
 */

// the sensor communicates using SPI, so include the library:
#include <SPI.h>

// pins used for the connection with the sensor
// the other you need are controlled by the SPI library):
//const int dataReadyPin = 6;
//const int chipSelectPin = 7;

const int ss_pin = 10;

SPISettings spisettings = SPISettings(40000000, MSBFIRST, SPI_MODE0);
typedef struct {
  float temperature;
  float pressure;
  float humidity;
  float north_direction;
} sensor_data_t;

sensor_data_t *sensor_data;

void setup() {
  Serial.begin(9600);

  // sensor data struct initialization
  sensor_data = malloc(sizeof(sensor_data_t));
  sensor_data->temperature = sensor_data->pressure = sensor_data->humidity = sensor_data->north_direction = 0.0;
  // sensor_data = (sensor_data_t) {.temperature = 0.0, .pressure = 0.0, .humidity = 0.0, .north_direction = 0.0};

  // setting SS pin value
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  // start the SPI library:
  SPI.begin();

  // initalize the  data ready and chip select pins:
  // pinMode(dataReadyPin, INPUT);
  // pinMode(chipSelectPin, OUTPUT);
}

void loop() {
  readSensorData();  

  // Send data to website
}

void readSensorData(){
  byte sent_data[sizeof(sensor_data)];
  char message[100];
  
  Serial.println("Reading data structure: ");
  SPI.beginTransaction(spisettings);
  digitalWrite(ss_pin, LOW);
  
  for(int i=0; i<sizeof(sensor_data); i+=8){
    sent_data[i] = SPI.transfer(0);
    // Serial.print(recv_data[i], BIN);
    // shifting 8 bits and using OR to copy the recv_data[i] bits on the last 8 bits of the variable
    // sensor_data.temperature << 8 | recv_data[i];
  }
  
  sensor_data = (sensor_data_t *) sent_data;
  Serial.println(snprintf(message, sizeof(message), "Temperature: %f °C", sensor_data->temperature));
  Serial.println(snprintf(message, sizeof(message), "Pressure: %f hPa", sensor_data->pressure));
  Serial.println(snprintf(message, sizeof(message), "Humidity: %f\%", sensor_data->humidity));
  Serial.println(snprintf(message, sizeof(message), "North direction: %f°", sensor_data->north_direction));

  digitalWrite(ss_pin, HIGH);
  SPI.endTransaction();  
}

//void readTemperature(){
//  byte recv_data[4];
//  
//  Serial.print("Temperature reading: ");
//  SPI.beginTransaction(spisettings);
//  digitalWrite(ss_pin, LOW);
//  
//  for(int i=0; i<4; i++){
//    recv_data[i] = SPI.transfer(0);
//    Serial.print(recv_data[i], BIN);
//    // shifting 8 bits and using OR to copy the recv_data[i] bits on the last 8 bits of the variable
//    sensor_data.temperature << 8 | recv_data[i];
//  }
//
//  Serial.println(" (that is " + sensor_data.temperature + "°C)");
//
//  digitalWrite(ss_pin, HIGH);
//  SPI.endTransaction();
//}
//
//void readPressure(){
//  byte recv_data[4];
//
//  SPI.beginTransaction(spisettings);
//  digitalWrite(ss_pin, LOW);
//  
//  Serial.print("Pressure reading: ");
//  
//  for(int i=0; i<4; i++){
//    recv_data[i] = SPI.transfer(0);
//    Serial.print(recv_data[i], BIN);
//    // shifting 8 bits and using OR to copy the recv_data[i] bits on the last 8 bits of the variable
//    sensor_data.pressure << 8 | recv_data[i];
//  }
//
//  Serial.println(" (that is " + sensor_data.pressure + "hPa)");
//
//  digitalWrite(ss_pin, HIGH);
//  SPI.endTransaction();
//}
//
//void readHumidity(){
//  byte recv_data[4];
//
//  SPI.beginTransaction(spisettings);
//  digitalWrite(ss_pin, LOW);
//
//  Serial.print("Humidity reading: ");
//  
//  for(int i=0; i<4; i++){
//    recv_data[i] = SPI.transfer(0);
//    Serial.print(recv_data[i], BIN);
//    // shifting 8 bits and using OR to copy the recv_data[i] bits on the last 8 bits of the variable
//    sensor_data.humidity << 8 | recv_data[i];
//  }
//
//  Serial.println(" (that is " + sensor_data.humidity + "%)");
//
//  digitalWrite(ss_pin, HIGH);
//  SPI.endTransaction();
//}
//
//void readNorthDirection(){
//  byte recv_data[4];
//
//  SPI.beginTransaction(spisettings);
//  digitalWrite(ss_pin, LOW);
//
//  Serial.print("North direction reading: ");
//  
//  for(int i=0; i<4; i++){
//    recv_data[i] = SPI.transfer(0);
//    Serial.print(recv_data[i], BIN);
//    // shifting 8 bits and using OR to copy the recv_data[i] bits on the last 8 bits of the variable
//    sensor_data.north_direction << 8 | recv_data[i];
//  }
//
//  Serial.println(" (that is " + sensor_data.north_direction + "°)");
//
//  digitalWrite(ss_pin, HIGH);
//  SPI.endTransaction();
//}
