// *Interfacing RGB LED with Arduino 
// * Author: Osama Ahmed 

//Defining  variable and the GPIO pin on Arduino
int redPin= 9;
int greenPin = 10;
int  bluePin = 11;

void setup() {
  //Defining the pins as OUTPUT
  pinMode(redPin,  OUTPUT);              
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}
void  loop() {
  setColor(255, 0, 0, 0.2); // Red Color
  delay(1000);
  setColor(0,  255, 0, 0.2); // Green Color
  delay(1000);
  setColor(0, 0, 255, 0.2); // Blue Color
  delay(1000);
  setColor(255, 255, 255, 0.2); // White Color
  delay(1000);
  setColor(170, 0, 255, 0.2); // Purple Color
  delay(1000);
  setColor(127, 127,  127, 0.2); // Light Blue
  delay(1000);
}
void setColor(int redValue, int greenValue,  int blueValue, float brightness) {
  analogWrite(redPin, brightness*redValue);
  analogWrite(greenPin, brightness*greenValue);
  analogWrite(bluePin, brightness*blueValue);
}
