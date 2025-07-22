## ✅ What You’ve Implemented:

### 🧠 Core Logic:

- Water flow is detected → `relayClosed = true` → contact is closed (`RELAY_FLOW_CONTACTS_PIN = LOW`)
- Water flow stops → `relayClosed = false` → contact is opened

### 🔒 Safety Logic:

- Flow is too weak (`< FLOW_MIN_HZ`) **or** contact is closed but no water for 5 seconds → `cutPower()` is called:
  - cuts power (`RELAY_POWER_BOARD_PIN = HIGH`)
  - triggers a watchdog reset via `WDTO_15MS`

### 🔁 After Reset:

- Power is restored
- The system restarts with a self-test
