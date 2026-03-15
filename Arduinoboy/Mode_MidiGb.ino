/**************************************************************************
 * Name:    Timothy Lamb                                                  *
 * Email:   trash80@gmail.com                                             *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

void modeMidiGbSetup()
{
  modeChangeRequested = false;
  midiPcWait          = false;
  midiValueMode       = false;
  midiAddressMode     = false;

  digitalWrite(pinStatusLed, LOW);
  pinMode(pinGBClock,    OUTPUT);
  digitalWrite(pinGBClock, HIGH);

#ifdef USE_TEENSY
  usbMIDI.setHandleRealTimeSystem(NULL);
#endif

  blinkMaxCount = 1000;
  modeMidiGb();
}

void modeMidiGb()
{
  boolean sendByte = false;
  while (1) {
    // ── USB MIDI (both USB and hardware paths active simultaneously) ──────
    modeMidiGbUsbMidiReceive();
    if (modeChangeRequested) break;

    // ── Hardware serial MIDI ──────────────────────────────────────────────
    if (serial->available()) {
      incomingMidiByte = serial->read();

      // ── Mode switch: PC data byte pending from ch16 ───────────────────
      if (midiPcWait && !(incomingMidiByte & 0x80)) {
        midiPcWait = false;
        handleModeChange(incomingMidiByte);
        if (modeChangeRequested) break;
        continue;
      }

      if (incomingMidiByte & 0x80) {
        // ── Mode switch: PC status byte on ch16 (0xCF) ───────────────────
        if ((incomingMidiByte & 0xF0) == 0xC0 &&
            (incomingMidiByte & 0x0F) == MODE_SWITCH_CH) {
          midiPcWait      = true;
          midiValueMode   = false;
          midiAddressMode = false;
          continue;
        }
        midiPcWait = false; // any other status byte cancels pending PC wait

        // ── mGB status byte routing ───────────────────────────────────────
        switch (incomingMidiByte & 0xF0) {
          case 0xF0:
            midiValueMode = false;
            break;
          default:
            sendByte = false;
            midiStatusChannel = incomingMidiByte & 0x0F;
            midiStatusType    = incomingMidiByte & 0xF0;
            if      (midiStatusChannel == memory[MEM_MGB_CH])   { midiData[0] = midiStatusType;     sendByte = true; }
            else if (midiStatusChannel == memory[MEM_MGB_CH+1]) { midiData[0] = midiStatusType + 1; sendByte = true; }
            else if (midiStatusChannel == memory[MEM_MGB_CH+2]) { midiData[0] = midiStatusType + 2; sendByte = true; }
            else if (midiStatusChannel == memory[MEM_MGB_CH+3]) { midiData[0] = midiStatusType + 3; sendByte = true; }
            else if (midiStatusChannel == memory[MEM_MGB_CH+4]) { midiData[0] = midiStatusType + 4; sendByte = true; }
            else { midiValueMode = false; midiAddressMode = false; }
            if (sendByte) {
              statusLedOn();
              sendByteToGameboy(midiData[0]);
              delayMicroseconds(GB_MIDI_DELAY);
              midiValueMode   = false;
              midiAddressMode = true;
            }
            break;
        }
      } else if (midiAddressMode) {
        midiAddressMode = false;
        midiValueMode   = true;
        midiData[1]     = incomingMidiByte;
        sendByteToGameboy(midiData[1]);
        delayMicroseconds(GB_MIDI_DELAY);
      } else if (midiValueMode) {
        midiData[2]     = incomingMidiByte;
        midiAddressMode = true;
        midiValueMode   = false;
        sendByteToGameboy(midiData[2]);
        delayMicroseconds(GB_MIDI_DELAY);
        statusLedOn();
        blinkLight(midiData[0], midiData[2]);
      }
    } else {
      updateBlinkLights();
      updateStatusLed();
    }
  }
}

 /*
 sendByteToGameboy does what it says. yay magic
 */
void sendByteToGameboy(byte send_byte)
{
 for(countLSDJTicks=0;countLSDJTicks!=8;countLSDJTicks++) {  //we are going to send 8 bits, so do a loop 8 times
   if(send_byte & 0x80) {
       GB_SET(0,1,0);
       GB_SET(1,1,0);
   } else {
       GB_SET(0,0,0);
       GB_SET(1,0,0);
   }

#if defined (F_CPU) && (F_CPU > 24000000)
   // Delays for Teensy etc where CPU speed might be clocked too fast for cable & shift register on gameboy.
   delayMicroseconds(1);
#endif
   send_byte <<= 1;
 }
}

void modeMidiGbUsbMidiReceive()
{
#ifdef USE_TEENSY
  while (usbMIDI.read()) {
    // PC on ch16 is handled globally by usbHandleProgramChange() callback.
    // We still need to check modeChangeRequested and stop processing if set.
    if (modeChangeRequested) return;

    uint8_t ch = usbMIDI.getChannel() - 1; // convert to 0-indexed
    boolean send = false;
    uint8_t gbCh = 0;
    if      (ch == memory[MEM_MGB_CH])   { gbCh = 0; send = true; }
    else if (ch == memory[MEM_MGB_CH+1]) { gbCh = 1; send = true; }
    else if (ch == memory[MEM_MGB_CH+2]) { gbCh = 2; send = true; }
    else if (ch == memory[MEM_MGB_CH+3]) { gbCh = 3; send = true; }
    else if (ch == memory[MEM_MGB_CH+4]) { gbCh = 4; send = true; }
    if (!send) continue; // not an mGB channel — skip

    uint8_t s;
    switch (usbMIDI.getType()) {
      case 0x80: // Note Off
        sendByteToGameboy(0x80 + gbCh); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData1()); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData2()); delayMicroseconds(GB_MIDI_DELAY);
        break;
      case 0x90: // Note On
        sendByteToGameboy(0x90 + gbCh); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData1()); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData2()); delayMicroseconds(GB_MIDI_DELAY);
        blinkLight(0x90 + gbCh, usbMIDI.getData2());
        break;
      case 0xB0: // CC
        sendByteToGameboy(0xB0 + gbCh); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData1()); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData2()); delayMicroseconds(GB_MIDI_DELAY);
        blinkLight(0xB0 + gbCh, usbMIDI.getData2());
        break;
      case 0xC0: // Program Change (mGB channels only — ch16 handled by callback)
        sendByteToGameboy(0xC0 + gbCh); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData1()); delayMicroseconds(GB_MIDI_DELAY);
        break;
      case 0xE0: // Pitch Bend
        sendByteToGameboy(0xE0 + gbCh); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData1()); delayMicroseconds(GB_MIDI_DELAY);
        sendByteToGameboy(usbMIDI.getData2()); delayMicroseconds(GB_MIDI_DELAY);
        break;
    }
    statusLedOn();
  }
#endif
}
