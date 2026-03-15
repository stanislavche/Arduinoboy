// Mode.ino — S_TN fork
// 3 modes: 0=mGB  1=LSDJ Slave Sync  2=Nanoloop Slave
// Switching via MIDI Program Change on channel 16

// ── switchMode: dispatch to the active mode ───────────────────────────────
void switchMode() {
  switch (memory[MEM_MODE]) {
    case 0:  modeMidiGbSetup();        break;
    case 1:  modeLSDJSlaveSyncSetup(); break;
    case 2:  modeNanoloopSetup();      break;
    default: memory[MEM_MODE] = 0; modeMidiGbSetup(); break;
  }
}

// ── handleModeChange: called on PC ch16 from USB or hardware MIDI ─────────
// Validates value 0-2, saves to EEPROM, triggers LED blink, signals break.
boolean handleModeChange(byte pc) {
  if (pc >= NUMBER_OF_MODES) return false;
  if (pc == memory[MEM_MODE])  return false; // already in this mode
  memory[MEM_MODE] = pc;
  if (!memory[MEM_FORCE_MODE]) EEPROM.write(MEM_MODE, pc);
  showSelectedMode(); // N-blink LED pattern
  modeChangeRequested = true;
  return true;
}

// ── Sequencer helpers ─────────────────────────────────────────────────────
void sequencerStart() {
  sequencerStarted   = true;
  countSyncPulse     = 0;
  countSyncTime      = 0;
  countSyncLightTime = 0;
  switchLight        = 0;
}

void sequencerStop() {
  midiSyncEffectsTime = false;
  sequencerStarted    = false;
  countSyncPulse      = 0;
  countSyncTime       = 0;
  countSyncLightTime  = 0;
  switchLight         = 0;
  digitalWrite(pinStatusLed, LOW);
}
