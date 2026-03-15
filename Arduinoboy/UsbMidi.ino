// UsbMidi.ino — S_TN fork
// USB MIDI support via Teensyduino usbMIDI (Teensy 2.0 + 3.x)
// Program Change on channel 16 → mode switching via handleModeChange()
// Real-time (clock/transport) handlers are registered by each mode's setup().

#ifndef USE_TEENSY
// ── Stubs for non-Teensy builds ───────────────────────────────────────────
void usbMidiInit()   {}
void usbMidiUpdate() {}
#else
// ── Teensy USB MIDI ───────────────────────────────────────────────────────

// Called by usbMIDI.read() for any Program Change message.
// If channel == 16: switch mode.  Otherwise: ignored (mGB handles ch 1-5 inline).
void usbHandleProgramChange(byte channel, byte number) {
  if (channel == MODE_SWITCH_CH + 1) {   // usbMIDI is 1-indexed; ch16 = index 15+1
    handleModeChange(number);
  }
}

void usbMidiSendRTMessage(uint8_t b) {
  usbMIDI.sendRealTime((usbMIDI_::MidiType)b);
}

void usbMidiInit() {
  usbMIDI.setHandleProgramChange(usbHandleProgramChange);
}

void usbMidiUpdate() {
  usbMIDI.read();
}

#endif



#endif






















