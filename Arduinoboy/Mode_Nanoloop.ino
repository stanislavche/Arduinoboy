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

void modeNanoloopSetup()
{
  modeChangeRequested = false;
  midiPcWait          = false;
  nanoState           = false;
  nanoSkipSync        = false;

  digitalWrite(pinStatusLed, LOW);
  pinMode(pinGBClock,    OUTPUT);
  digitalWrite(pinGBClock, HIGH);

#ifdef USE_TEENSY
  usbMIDI.setHandleRealTimeSystem(usbMidiNanoloopRealtimeMessage);
#endif

  blinkMaxCount = 1000;
  modeNanoloopSync();
}

void modeNanoloopSync()
{
  while (1) {
    // ── USB MIDI: realtime via callback, PC ch16 via usbHandleProgramChange
    modeNanoloopUsbMidiReceive();
    if (modeChangeRequested) break;

    // ── Hardware serial MIDI ───────────────────────────────────────────────
    if (serial->available()) {
      incomingMidiByte = serial->read();

      // Mode switch: PC data byte pending from ch16
      if (midiPcWait && !(incomingMidiByte & 0x80)) {
        midiPcWait = false;
        handleModeChange(incomingMidiByte);
        if (modeChangeRequested) break;
        continue;
      }

      if (incomingMidiByte & 0x80) {
        // Mode switch: PC status on ch16 (0xCF)
        if ((incomingMidiByte & 0xF0) == 0xC0 &&
            (incomingMidiByte & 0x0F) == MODE_SWITCH_CH) {
          midiPcWait = true;
          continue;
        }
        midiPcWait = false;

        switch (incomingMidiByte) {
          case 0xF8: // Clock
            if (sequencerStarted) {
              nanoSkipSync = !nanoSkipSync;
              if (countSyncTime) {
                nanoState = sendTickToNanoloop(nanoState, false);
              } else {
                nanoState = sendTickToNanoloop(true, true);
              }
              nanoState = sendTickToNanoloop(nanoState, nanoSkipSync);
              updateVisualSync();
            }
            break;
          case 0xFA: // Start
          case 0xFB: // Continue
            sequencerStart();
            break;
          case 0xFC: // Stop
            sequencerStop();
            break;
          default:
            break;
        }
      }
    }
    if (modeChangeRequested) break;
    updateStatusLight();
  }
}

boolean sendTickToNanoloop(boolean state, boolean last_state)
{
  if(!state) {
    if(last_state) {
       GB_SET(0,1,0);
       GB_SET(1,1,0);
    } else {
       GB_SET(0,0,0);
       GB_SET(1,0,0);
    }
    return true;
  } else {
    GB_SET(0,1,0);
    GB_SET(1,1,0);
    return false;
  }
}

void usbMidiNanoloopRealtimeMessage(uint8_t message)
{
    switch(message) {
      case 0xF8:
          if(sequencerStarted) {
            nanoSkipSync = !nanoSkipSync;
            if(countSyncTime) {
              nanoState = sendTickToNanoloop(nanoState, false);
            } else {
              nanoState = sendTickToNanoloop(true, true);
            }
            nanoState = sendTickToNanoloop(nanoState, nanoSkipSync);
            updateVisualSync();
          }
      break;
      case 0xFA:                                // Case: Transport Start Message
      case 0xFB:                                // and Case: Transport Continue Message
          sequencerStart();                       // Start the sequencer
      break;
      case 0xFC:                                // Case: Transport Stop Message
          sequencerStop();
      break;
    }
}


// USB MIDI receive for Nanoloop mode.
// Real-time is handled by usbMidiNanoloopRealtimeMessage callback.
// PC on ch16 is handled by usbHandleProgramChange callback.
void modeNanoloopUsbMidiReceive()
{
#ifdef USE_TEENSY
  while (usbMIDI.read()) {
    if (modeChangeRequested) return;
    // All meaningful messages handled via callbacks; nothing else needed here.
  }
#endif

}
