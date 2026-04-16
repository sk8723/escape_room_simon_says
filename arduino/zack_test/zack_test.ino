#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Servo.h>   // <-- Add this

// Use pins 2 and 3 to communicate with DFPlayer Mini
static const uint8_t PIN_MP3_TX = 3; // Connects to module's RX
static const uint8_t PIN_MP3_RX = 2; // Connects to module's TX
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);

// Create the Player object
DFRobotDFPlayerMini player;

// Create Servo object
Servo myServo;
const int SERVO_PIN = 6;  // Servo connected to pin 9

void setup() {
  // LEDs
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);

  // Attach servo
  myServo.attach(SERVO_PIN);

  
  // Init USB serial port for debugging
  Serial.begin(9600);
  // Init serial port for DFPlayer Mini
  softwareSerial.begin(9600);
  delay(2000);

  // Start communication with DFPlayer Mini
  if (player.begin(softwareSerial)) {
    Serial.println("OK");

    // Set volume to maximum (0 to 30).
    player.volume(15);
    // Play the "0001.mp3" in the "mp3" folder on the SD card
    //player.playMp3Folder(1);
    player.play(1);

  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }
}

void loop() {
  while(1) {

    myServo.write(0);
    delay(250);

    digitalWrite(9, HIGH);
    delay(250);
    digitalWrite(10, HIGH);
    delay(250);
    digitalWrite(11, HIGH);
    delay(250);
    digitalWrite(12, HIGH);
    delay(250);

    myServo.write(90);
    delay(250);
    
    digitalWrite(9, LOW);
    delay(250);
    digitalWrite(10, LOW);
    delay(250);
    digitalWrite(11, LOW);
    delay(250);
    digitalWrite(12, LOW);
    delay(250);
  }
}