#include <avr/sleep.h>
#include <avr/wdt.h>

const byte flowSensorPin = 2;
const byte relayPin = 4;

const unsigned long flowInterval = 500;
const int pulseThreshold = 4;

unsigned long flowTimer = 0;
volatile byte count = 0; // Only goes up to 255 but is sufficient for household pressure

bool relayOn = false;

void setup() {
  Serial.begin(9600);

  // Check if watchdog caused reset
  if (MCUSR & _BV(WDRF)) {
    Serial.println("⚠️ Last reset was from watchdog!");
  }
  MCUSR = 0; // Clear reset flags

  pinMode(flowSensorPin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Relay OFF initially

  attachInterrupt(digitalPinToInterrupt(flowSensorPin), wakeUp, RISING);

  ADCSRA &= ~(1 << ADEN); // Disable ADC

  // Enable watchdog reset after 2 seconds
  wdt_enable(WDTO_2S);

  Serial.println("Flow detector initialized. Going to sleep...");
  Serial.flush();  // Wait for Serial output to finish

  toSleep();
}

void loop() {
  unsigned long now = millis();

  // Check every 500 ms
  if (now - flowTimer >= flowInterval) {
    flowTimer = now;

    wdt_reset(); // Prevent watchdog reset (we’re alive)

    // Copy and reset pulse count safely
    noInterrupts();
    int pulseCount = count;
    count = 0;
    interrupts();

    Serial.print("Pulses in last ");
    Serial.print(flowInterval);
    Serial.print(" ms: ");
    Serial.println(pulseCount);

    if (pulseCount > pulseThreshold) {
      if (!relayOn) {
        digitalWrite(relayPin, LOW);  // Relay ON
        relayOn = true;
        Serial.println("Flow detected — relay ON.");
      }
    } else {
      if (relayOn) {
        digitalWrite(relayPin, HIGH); // Relay OFF
        relayOn = false;
        Serial.println("Flow stopped — relay OFF. Going to sleep...");
      } else {
        Serial.println("Flow too low — staying asleep.");
      }

      toSleep(); // Go back to sleep
    }
  }
}

void toSleep() {
  delay(20); // Allow relay to settle

  // Stop watchdog to avoid reset during sleep
  wdt_disable();

  // Clear any pending external interrupt
  EIFR = bit(INTF0);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();    // Sleep

  // Resumes here after wake-up
  sleep_disable();

  // Re-enable watchdog after waking
  wdt_enable(WDTO_2S);

  Serial.println("Woke up from sleep.");
}

void wakeUp() {
  count++;
}
