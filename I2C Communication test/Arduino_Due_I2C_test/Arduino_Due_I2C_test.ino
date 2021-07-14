/*Arduino 1 code
  Example of code for use I2C communication between two Arduino :

  Arduino 1                 Secondary Arduino
    A4                         A4
    A5                         A4
    GND                        GND
*/

// Include the required Wire library for I2C / integrer la librairie Wire pour utiliser l I2C
#include <Wire.h> 

int x = 0; // creation of the variable x for stockage the state of the led on the secondary arduino / creation d une variable x qui stocke l etat de la led sur l arduino secondaire

void setup() {
  // Start the I2C Bus / innitialisation du bus I2C
  Wire.begin();
}

void loop() {
  Wire.beginTransmission(0x09);  // transmit to device #9 / transmission sur l arduino secondaire a l adresse 0x09 (=9 en decimale)
  Wire.write(x);                 // sends x / envoi de la valeur de x
  Wire.endTransmission();        // stop transmitting / arret de la transmission
  x++;                           // Increment x / incremente x
  delay(500);                    // delay of 500  milli seconde between two send / delay de 500 milli seconde entre 2 envoi sur le bus I2C
}
