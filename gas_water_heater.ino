// === Gas Water Heater Controller ===
//
// 🔵 LED Indication Reference:
// - 3 short blinks (0.1 sec) — start of self-test
// - 1 long blink (0.6 sec) — self-test passed successfully
// - 5 fast blinks (0.2 sec) — ❌ error: flow sensor is not generating pulses
// - Every 10 liters of water — 2 quick blinks

// - Relay logic is designed for inverted relays

#define DEBUG 1  // Set to 0 to disable Serial output

#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// === Pins ===
const int FLOW_SENSOR_PIN         = 2;
const int LED_PIN                 = 5;
const int RELAY_FLOW_CONTACTS_PIN = 6;  // closes "turn on water" contact
const int RELAY_POWER_BOARD_PIN   = 7;  // powers the heater's internal board

// === Configuration ===
const unsigned long NO_FLOW_TIMEOUT_MS   = 5000;
const unsigned int FLOW_MIN_HZ           = 2;      // Minimum pulses per second ≈ 0.18 L/min (2 / 11)
const float HZ_PER_LPM                   = 11.0;   // YF-B1: F = 11 × Q (Q = L/min)
const float PULSES_PER_LITER             = HZ_PER_LPM / 60.0;

const int LED_SHORT               = 100;
const int LED_LONG                = 600;
const int LED_ERROR               = 200;
const int SELFTEST_SENSOR_DELAY   = 300;

volatile unsigned long pulseCount = 0;
volatile bool flowDetected        = false;

unsigned long lastFlowTime        = 0;
unsigned long lastLitersMark      = 0;
bool relayClosed                  = false;

// === Interrupt on flow pulse ===
void flowInterrupt() {
  pulseCount++;
  lastFlowTime = millis(); // updated only on initial detection
  flowDetected = true;
}

// === Enter deep sleep until next interrupt ===
void goToSleep() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  noInterrupts();
  sleep_bod_disable();  // disable brown-out detector: отключить регулятор напряжения сна (BOD)
  interrupts();
  sleep_cpu();
  sleep_disable();
}

// Uses delay(), acceptable here as it's rarely called.
// Used for self-test, error indication, and flow volume events — doesn’t interfere with flow logic.
void blink(int pin, int times, int duration = LED_SHORT) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
    delay(duration);
  }
}

// === Power cut-off and system reset ===
void cutPower() {
  // For inverted relays: LOW = ON, HIGH = OFF
  digitalWrite(RELAY_FLOW_CONTACTS_PIN, HIGH);    // open the contact
  digitalWrite(RELAY_POWER_BOARD_PIN, HIGH);      // cut power to heater
  delay(100);
  wdt_enable(WDTO_15MS);  // short watchdog reset
  while (true) {          // wait for reset
    delay(10);            // avoid CPU overload
  }          
}

// === Debug output for current flow status ===
void printFlowStatus(unsigned long pulsesThisSec, unsigned long totalPulses) {
#if DEBUG
  float flowLPM = pulsesThisSec / HZ_PER_LPM;
  float totalLiters = totalPulses / PULSES_PER_LITER;

  Serial.print("Flow: ");
  Serial.print(flowLPM);
  Serial.print(" L/min, Total: ");
  Serial.print(totalLiters);
  Serial.println(" L");
#endif
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_FLOW_CONTACTS_PIN, OUTPUT);
  pinMode(RELAY_POWER_BOARD_PIN, OUTPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT);

  // Inverted relay logic: LOW = ON, HIGH = OFF
  digitalWrite(RELAY_POWER_BOARD_PIN, HIGH);   // disable power on boot
  digitalWrite(RELAY_FLOW_CONTACTS_PIN, HIGH); // open the contacts

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowInterrupt, RISING);

#if DEBUG
  Serial.begin(9600);
  Serial.println("Booting... running self-test");
#endif

  blink(LED_PIN, 3, LED_SHORT);   // self-test start
  pulseCount = 0;                 // reset before sensor test
  delay(SELFTEST_SENSOR_DELAY);

  if (pulseCount == 0) {
#if DEBUG
    Serial.println("❌ Error: flow sensor not producing pulses");
#endif
    blink(LED_PIN, 5, LED_ERROR);
  }

  blink(LED_PIN, 1, LED_LONG);              // self-test passed
  digitalWrite(RELAY_POWER_BOARD_PIN, LOW); // supply power to heater
  lastFlowTime = millis();                  // initialize flow timer
  wdt_enable(WDTO_2S);                      // 2s watchdog
}

void loop() {
  wdt_reset();
  unsigned long now = millis();
  
  // Wait for flow detection
  while (!flowDetected) {
    goToSleep();
  }
  flowDetected = false; // reset flag

  static unsigned long lastPulseSnapshot = 0;
  static unsigned long lastFreqCheck = 0;

  // Check flow every second
  if (now - lastFreqCheck >= 1000) {
    unsigned long pulsesThisSec = pulseCount - lastPulseSnapshot;
    lastPulseSnapshot = pulseCount;
    lastFreqCheck = now;

    printFlowStatus(pulsesThisSec, pulseCount);

    if (pulseCount - lastLitersMark >= (unsigned long)(10 * PULSES_PER_LITER)) {
      blink(LED_PIN, 2, LED_SHORT);
      lastLitersMark = pulseCount;
    }

    if (relayClosed && pulsesThisSec < FLOW_MIN_HZ) {
#if DEBUG
      Serial.println("❗ Flow too weak — cutting heater power");
#endif
      cutPower();
    }
  }

  // Engage relay when water starts flowing
  if (!relayClosed && now - lastFlowTime < 1000) {
    digitalWrite(RELAY_FLOW_CONTACTS_PIN, LOW); // замкнуть
    relayClosed = true;
#if DEBUG
    Serial.println("Contact closed — heater starting");
#endif
  }

  // Release relay when flow stops
  if (relayClosed && now - lastFlowTime > 1000) {
    digitalWrite(RELAY_FLOW_CONTACTS_PIN, HIGH);
    relayClosed = false;
#if DEBUG
    Serial.println("Flow stopped — contact opened");
#endif
  }

  // Accident: heater is ON, but no flow for too long
  if (relayClosed && now - lastFlowTime > NO_FLOW_TIMEOUT_MS) {
#if DEBUG
    Serial.println("⚠️ Error: heater ON but no water flow — cutting power!");
#endif
    cutPower();
  }

  // 🔋 Light sleep between loop iterations
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_mode();
  sleep_disable();
} 
