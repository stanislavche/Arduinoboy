// Led_Functions.ino — S_TN fork
// Single LED: onboard Teensy 2.0 LED on pinStatusLed (pin 11 / PD6)
//
// showSelectedMode  – blinks N times on mode switch
//                     1 blink = mGB (mode 0)
//                     2 blinks = LSDJ Slave (mode 1)
//                     3 blinks = Nanoloop (mode 2)
// startupSequence   – 3 quick blinks on power-up
// updateVisualSync  – blinks once per quarter-note (called from clock tick)
// blinkLight        – brief LED flash on any mGB MIDI activity
// statusLedOn       – turn LED on, used by mGB activity flash
// updateStatusLed   – called each loop to time-out statusLedOn
// updateBlinkLights / updateStatusLight – unified single-LED blink decay

// ─────────────────────────────────────────────────────────────────────────

void showSelectedMode() {
  int n = (int)memory[MEM_MODE] + 1; // 1 / 2 / 3 blinks
  for (int i = 0; i < n; i++) {
    digitalWrite(pinStatusLed, HIGH);
    delay(150);
    digitalWrite(pinStatusLed, LOW);
    delay(150);
  }
  delay(300);
  lastMode = memory[MEM_MODE];
}

// ── Startup ───────────────────────────────────────────────────────────────
void startupSequence() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(pinStatusLed, HIGH);
    delay(80);
    digitalWrite(pinStatusLed, LOW);
    delay(80);
  }
  delay(200);
}

// ── Sync visual (called each MIDI clock tick in LSDJ / Nanoloop modes) ────
void updateVisualSync() {
  if (!countSyncTime) {
    statusLedOn();          // brief blink on every quarter-note downbeat
    switchLight++;
    if (switchLight == 4) switchLight = 0;
  }
  countSyncTime++;
  if (countSyncTime == 24) countSyncTime = 0;
}

// ── Single-LED blink decay ────────────────────────────────────────────────
void updateBlinkLights() {
  if (blinkSwitch) {
    blinkSwitchTime++;
    if (blinkSwitchTime >= blinkMaxCount) {
      blinkSwitch     = false;
      blinkSwitchTime = 0;
      digitalWrite(pinStatusLed, LOW);
    }
  }
}

// Alias kept so LSDJ/Nanoloop modes compile without changes
void updateStatusLight() {
  updateBlinkLights();
}

// ── mGB activity flash: any incoming MIDI message → brief LED blink ───────
void blinkLight(byte midiMessage, byte midiValue) {
  if (midiValue) {
    statusLedOn();
  }
}

// ── Status LED on/off (used by mGB to indicate byte-to-GB activity) ───────
void statusLedOn() {
  if (statusLedIsOn) {
    statusLedBlink = true;
  }
  statusLedIsOn    = true;
  countStatusLedOn = 0;
  digitalWrite(pinStatusLed, HIGH);
}

void updateStatusLed() {
  if (statusLedIsOn) {
    countStatusLedOn++;
    if (countStatusLedOn > 3000) {
      countStatusLedOn = 0;
      digitalWrite(pinStatusLed, LOW);
      statusLedIsOn = false;
    } else if (statusLedBlink && countStatusLedOn == 1) {
      digitalWrite(pinStatusLed, LOW);
    } else if (statusLedBlink && countStatusLedOn > 1000) {
      statusLedBlink = false;
      digitalWrite(pinStatusLed, HIGH);
    }
  }
}