# 💧 Flow Sensor Relay Controller with Sleep Mode

This Arduino sketch monitors a flow sensor and controls a relay based on water flow detection. It uses low-power techniques by sleeping the microcontroller when flow is not detected and waking on sensor pulses (interrupts). It also incorporates the watchdog timer for safe operation and auto-reset in case of hangs.

## 🚀 Features

- 💤 **Power-efficient**: Uses `SLEEP_MODE_PWR_DOWN` and disables ADC to reduce power draw
- 🔁 **Watchdog Reset**: Automatically resets after 2 seconds if the loop hangs
- ⚡ **Interrupt-driven flow detection**: Wakes on `RISING` signal from flow sensor
- 💡 **Relay control**: Activates a relay when flow exceeds a pulse threshold
- 🧠 **Failsafe logic**: Turns off relay and re-enters sleep mode if flow stops

## 🔌 Hardware Requirements

- Arduino (e.g. Uno, Nano)
- Hall-effect flow sensor connected to pin **D2**
- Relay module connected to pin **D4**
- Pull-up resistors (optional if sensor doesn't have internal pull-up)
- Power supply suited for the sensor and relay

## 📐 Pinout

| Component    | Arduino Pin |
| ------------ | ----------- |
| Flow Sensor  | D2          |
| Relay Module | D4          |

## ⚙️ Behavior

- Wakes from deep sleep when flow sensor pulses (via interrupt on D2)
- Every 500 ms, checks pulse count:
  - If pulses > 4, turns **relay ON**
  - If pulses ≤ 4, turns **relay OFF** and goes back to sleep
- Uses **watchdog timer** for safety and auto-restart after 2 seconds of inactivity

## 🔄 Pulse Logic

```cpp
if (pulseCount > 4) {
  // Sufficient flow: turn relay ON
} else {
  // Insufficient flow: turn relay OFF and sleep
}
```

## 🛠 Setup

1. Connect hardware according to the pinout
2. Upload the sketch to your Arduino
3. Open Serial Monitor at 9600 baud for logs
4. Run water through the flow sensor to observe activation

## 📝 Notes

- The sketch assumes the relay is **active LOW**
- `volatile byte count` is sufficient for typical water flow durations
- `wdt_reset()` is used to keep the microcontroller alive during active monitoring
- Sleep resumes only when flow is re-detected
