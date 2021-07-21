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
int x = 0;

void receiveEvent(int numBytes) {
  (void) numBytes;
  x = Wire.read();                // read one character from the I2C 
}

void setup() {
  pinMode (LED, OUTPUT);          // Define the LED pin as Output 

  Wire.begin(9);                  // Start the I2C Bus with address 9 in decimal (= 0x09 in hexadecimal) 

  Wire.onReceive(receiveEvent);   // Attach a function to trigger when something is received 
}

void loop() {

  //If value received is a multiple of 2  turn on the led 
  if (x % 2 == 0) {
    digitalWrite(LED, HIGH);
  }

  //Else turn off the led 
  else {
    digitalWrite(LED, LOW);
  }
} 
