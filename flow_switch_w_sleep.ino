#include <avr/sleep.h>

const byte flowSensorPin = 2;
const byte relayPin = 4;

unsigned long flowTimeShot = 0;
unsigned long flowInterval = 500;

unsigned long toSleepTimeShot = 0;
unsigned long toSleepInterval = 500;

volatile int count;

void setup() {
  pinMode(flowSensorPin, INPUT_PULLUP);

  digitalWrite(relayPin, HIGH);
  pinMode(relayPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(flowSensorPin), wakeUp, RISING);

  // Serial.begin(9600);
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - flowTimeShot >= flowInterval) {
    flowTimeShot = currentTime;

    if (count > 1) {
      digitalWrite(relayPin, LOW);
    } else {
      toSleep();
    }

    count = 0;
  }
}

void toSleep() {
  sleep_enable();
  interrupts();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  digitalWrite(relayPin, HIGH);
  delay(200);

  sleep_cpu();

  // Serial.println("I woke up!");
}

void wakeUp() {
  // Serial.println("Interrupt poked me!");

  sleep_disable();
  // noInterrupts();

  count++;
}