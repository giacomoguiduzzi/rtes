/*Arduino 1 code
  Example of code for use I2C communication between two Arduino :

  Arduino 1                 Secondary Arduino
    A4                         A4
    A5                         A4
    GND                        GND
*/

// Include the required Wire library for I2C / integrer la librairie Wire pour utiliser l I2C
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <MAX44009.h>

#define SEALEVELPRESSURE_HPA (1013.25)//< Average sea level pressure is 1013.25 hPa

typedef struct {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float brightness;
} sensors_data_t;

Adafruit_BME280 bme; //  I2C Define BME280
MAX44009 light_sensor;    //   I2C Define MAX44009
sensors_data_t sensors_data;
uint8_t *sent_data;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  // Start the I2C Bus / innitialisation du bus I2C
  Wire.begin();

  sent_data = (uint8_t *)&sensors_data;

  sensors_data.temperature = 0.0;
  sensors_data.humidity = 0.0;
  sensors_data.pressure = 0.0;
  sensors_data.altitude = 0.0;
  sensors_data.brightness = 0.0;

  
  bool status = bme.begin(0x76);  
  while(!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    delay(500);
    status = bme.begin(0x76);
  }
}

void loop() {
  sensors_data.temperature = bme.readTemperature();
  sensors_data.humidity = bme.readHumidity();
  sensors_data.pressure = bme.readPressure();
  sensors_data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  sensors_data.brightness = light_sensor.get_lux();

  Serial.println("------------------- Sending new data -------------------");

  Serial.print("Temperature: ");
  Serial.print(sensors_data.temperature);
  Serial.println("°C");

  Serial.print("Humidity: ");
  Serial.print(sensors_data.humidity);
  Serial.println("%");

  Serial.print("Pressure: ");
  Serial.print(sensors_data.pressure);
  Serial.println("hPa");

  Serial.print("Altitude: ");
  Serial.print(sensors_data.altitude);
  Serial.println("m");

  Serial.print("Brightness: ");
  Serial.print(sensors_data.brightness);
  Serial.println("lux");

  Wire.beginTransmission(0x33);  // transmit to device #9 / transmission sur l arduino secondaire a l adresse 0x09 (=9 en decimale)
  Wire.write(sent_data, sizeof(sensors_data));
  Wire.endTransmission();        // stop transmitting / arret de la transmission
  
  delay(500);                    // delay of 500  milli seconde between two send / delay de 500 milli seconde entre 2 envoi sur le bus I2C
}
