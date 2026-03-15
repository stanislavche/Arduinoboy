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

void modeLSDJSlaveSyncSetup()
{
  modeChangeRequested = false;
  midiPcWait          = false;
  midiNoteOnMode      = false;

  digitalWrite(pinStatusLed, LOW);
  pinMode(pinGBClock,    OUTPUT);
  digitalWrite(pinGBClock, HIGH);

#ifdef USE_TEENSY
  usbMIDI.setHandleRealTimeSystem(usbMidiLSDJSlaveRealtimeMessage);
#endif

  blinkMaxCount = 1000;
  modeLSDJSlaveSync();
}

void modeLSDJSlaveSync()
{
  while (1) {
    // ── USB MIDI: clock/transport via realtime callback, PC ch16 via usbHandleProgramChange
    modeLSDJSlaveSyncUsbMidiReceive();
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
          midiPcWait     = true;
          midiNoteOnMode = false;
          continue;
        }
        midiPcWait = false;

        switch (incomingMidiByte) {
          case 0xF8: // Clock
            if ((sequencerStarted && midiSyncEffectsTime && !countSyncTime)
                || (sequencerStarted && !midiSyncEffectsTime)) {
              sendClockTickToLSDJ();
              updateVisualSync();
            }
            if (midiSyncEffectsTime) {
              countSyncTime++;
              countSyncTime = countSyncTime % countSyncSteps;
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
            if (incomingMidiByte == (0x90 + memory[MEM_LSDJSLAVE_MIDI_CH])) {
              midiNoteOnMode = true;
              midiData[0]    = 0;
            } else {
              midiNoteOnMode = false;
            }
            break;
        }
      } else if (midiNoteOnMode) {
        if (!midiData[0]) {
          midiData[0] = incomingMidiByte;
        } else {
          if (incomingMidiByte > 0x00) getSlaveSyncEffect(midiData[0]);
          midiData[0] = 0;
        }
      }
    }
    if (modeChangeRequested) break;
    updateStatusLight();
  }
}

/*
  sendClockTickToLSDJ is a lovely loving simple function I wish they where all this short
  Technicallyly we are sending nothing but a 8bit clock pulse
*/
void sendClockTickToLSDJ()
{
  for(countLSDJTicks=0;countLSDJTicks<8;countLSDJTicks++) {
    GB_SET(0,0,0);
    GB_SET(1,0,0);
  }
}


/*
  getSlaveSyncEffect receives a note, and assigns the propper effect of that note
*/
void getSlaveSyncEffect(byte note)
{
    switch(note) {
    case 48:                        //C-3ish, Transport Start
      sequencerStart();
      break;
    case 49:                        //C#3 Transport Stop
      sequencerStop();
      break;
    case 50:                        //D-3 Turn off sync effects
      midiSyncEffectsTime = false;
      break;
    case 51:                        //D#3 Sync effect, 1/2 time
      midiSyncEffectsTime = true;
      countSyncTime = 0;
      countSyncSteps = 2;
      break;
    case 52:                        //E-3 Sync Effect, 1/4 time
      midiSyncEffectsTime = true;
      countSyncTime = 0;
      countSyncSteps = 4;
      break;
    case 53:                        //F-3 Sync Effect, 1/8 time
      midiSyncEffectsTime = true;
      countSyncTime = 0;
      countSyncSteps = 8;
      break;
    default:                        //All other notes will make LSDJ Start at the row number thats the same as the note number.
      midiDefaultStartOffset = midiData[0];
      break;
    }
}

void usbMidiLSDJSlaveRealtimeMessage(uint8_t message)
{
    switch(message) {
      case 0xF8:
          if((sequencerStarted && midiSyncEffectsTime && !countSyncTime)   //If the seq has started and our sync effect is on and at zero
            || (sequencerStarted && !midiSyncEffectsTime)) {               //or seq is started and there is no sync effects
                if(!countSyncPulse && midiDefaultStartOffset) {          //if we received a note for start offset
                  //sendByteToGameboy(midiDefaultStartOffset);              //send the offset
                }
                sendClockTickToLSDJ();                                   //send the clock tick
                updateVisualSync();
          }
          if(midiSyncEffectsTime) {                                      //If sync effects are turned on
            countSyncTime++;                                             //increment our tick counter
            countSyncTime = countSyncTime % countSyncSteps;              //and mod it by the number of steps we want for the effect
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


// USB MIDI receive for LSDJ Slave mode.
// Real-time (0xF8/FA/FB/FC) is handled by usbMidiLSDJSlaveRealtimeMessage callback.
// PC on ch16 is handled by usbHandleProgramChange callback (registered in usbMidiInit).
// Here we only catch NoteOn on the slave channel (sync effects).
void modeLSDJSlaveSyncUsbMidiReceive()
{
#ifdef USE_TEENSY
  while (usbMIDI.read()) {
    if (modeChangeRequested) return;
    byte ch   = usbMIDI.getChannel(); // 1-indexed
    byte type = usbMIDI.getType();
    if (ch == (byte)(memory[MEM_LSDJSLAVE_MIDI_CH] + 1) && type == 0x90) {
      getSlaveSyncEffect(usbMIDI.getData1());
    }
    // 0xC0 on ch16 is handled by usbHandleProgramChange callback — no action needed here
  }
#endif
}
