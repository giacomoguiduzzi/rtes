/*
Secondary arduino code
  Example of code for use I2C communication between two Arduino :

  Main Arduino (Uno)       Secondary Arduino (Uno)
     A4                         A4
     A5                         A4
    GND                         GND
*/
// Include the required Wire library for I2C 
#include <Wire.h> 

#define LED LED_BUILTIN

typedef struct {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float brightness;
} sensors_data_t;

sensors_data_t sensors_data;
uint8_t *sent_data;

void receiveEvent(size_t numBytes) {
  Serial.println("Triggered receiveEvent");
  digitalWrite(LED, LOW);
  for(uint8_t i=0; i<numBytes; i++)
    sent_data[i] = Wire.read();  // read one character from the I2C 
  digitalWrite(LED, HIGH);

  Serial.println("------------------- New data read -------------------");
  
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
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  pinMode (LED, OUTPUT);          // Define the LED pin as Output 
  digitalWrite(LED, HIGH);
  sent_data = (uint8_t *)&sensors_data;

  Wire.begin(0x33);                  // Start the I2C Bus with address 9 in decimal (= 0x09 in hexadecimal) 

  Wire.onReceive(receiveEvent);   // Attach a function to trigger when something is received 
}

void loop() {
  Serial.println("Waiting for data...");
  delay(1000);  
} 
