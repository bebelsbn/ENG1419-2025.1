#include <GY6050.h>
#include <Wire.h>
#include <Servo.h>

Servo myservoa;
Servo myservob;

int X = 0;
int Y = 0;

int gameStartPin = 8;

int offsetX = 0;
int offsetY = 0;

GY6050 gyro(0x68);

void setup() {
  Wire.begin();
  gyro.initialisation();
  delay(100);
  myservoa.attach(9);
  myservob.attach(6);
  myservoa.write(95);
  myservob.write(75);
  Serial.begin(9600);
  Serial.println("Type 'reset' to recalibrate the middle position.");

  pinMode(gameStartPin, INPUT);
}

void loop() {

  int gameState = digitalRead(gameStartPin);
  Serial.println(gameState);
  if (gameState == 1) {

    // Read raw values from accelerometer
    int rawX = gyro.refresh('A', 'X');
    int rawY = gyro.refresh('A', 'Y');

    // Apply offset
    int correctedX = rawX - offsetX;
    int correctedY = rawY - offsetY;

    // Map corrected values to servo range
    X = map(correctedX, -90, 90, 40, 100);
    Y = map(correctedY, -90, 90, 110, 50);

    myservoa.write(Y);
    myservob.write(X);

    // Check for serial input
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();  // Remove whitespace/newline

      if (input.equalsIgnoreCase("reset")) {
        offsetX = gyro.refresh('A', 'X');
        offsetY = gyro.refresh('A', 'Y');
        Serial.println("Recalibrated to current position.");
      }
    }

  } else {
    myservoa.write(95);
    myservob.write(75);
  }
  delay(15);
}
