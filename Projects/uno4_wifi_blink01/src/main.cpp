#include <Arduino.h>

void setup() {
  // initialize serial communication at 9600 bits per second
  Serial.begin(115200);
  Serial.println("Start process!!");

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // print out the state of the process
  Serial.println("LED update cycle.");

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                      // wait for a second
}
