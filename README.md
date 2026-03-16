# Arduinoboy — S_TN Fork
### Teensy 2.0 inside DMG Game Boy · MIDI via 3.5 mm TRS · USB MIDI

> **Fork of** [trash80/Arduinoboy](https://github.com/trash80/Arduinoboy) — stripped down and rebuilt for **Teensy 2.0** installed directly inside a DMG-01 Game Boy.  
> MIDI input: hardware serial (3.5 mm TRS Type A, M8-compatible) **and** USB MIDI simultaneously.

---

## What it does

Bridges your MIDI sequencer / [Dirtywave M8](https://dirtywave.com) to the Game Boy's link port — turning a stock DMG into a MIDI sound module or sync target, powered by the Game Boy's own batteries.

| Mode           | PC # | GB software required                  | What you control              |
|----------------|------|---------------------------------------|-------------------------------|
| **mGB**        | 0    | [mGB](https://github.com/trash80/mGB) cartridge | Full MIDI → 5 GB channels |
| **LSDJ Slave** | 1    | [LSDJ](https://www.littlesounddj.com) (sync: Slave) | Clock, Start/Stop, tempo multiplier |
| **Nanoloop**   | 2    | [Nanoloop](https://www.nanoloop.com) (sync: Slave) | Clock sync                    |

---

## Hardware

| Component | Details |
|-----------|---------|
| **Teensy 2.0** | ATmega32U4, 5 V, 16 MHz — soldered inside DMG shell |
| **6N138** | Optocoupler for MIDI IN galvanic isolation |
| **1N5819** | Schottky diode, reverse polarity protection |
| **220 Ω** | Current limit resistor (6N138 LED) |
| **270 Ω** | Pull-up resistor (6N138 output) |
| **3.5 mm TRS socket** | Stereo jack, MIDI IN TRS Type A (M8 compatible) |
| **~30 cm wire** | 6 conductors AWG 28–30 for link port |

**Power:** from DMG link port pin 1 (+5 V). No batteries or external PSU needed.  
**USB:** used for firmware flashing only — disconnect GB batteries when flashing.

📄 Full wiring diagrams, pinouts, and component alternatives:
- [`Instruction.md`](Instruction.md) — English
- [`Instruction.ru.md`](Instruction.ru.md) — Русский

---

## DMG Link Port — Quick Reference

```
  SOLDER SIDE (as seen from inside GB when soldering):

  left edge ──────────────────────── right edge
  ┌──────────────────────────────────────────┐
  │  [1 VCC]     [3 SI]     [5 SCK]          │  ← top row
  │  ─────────────────────────────           │
  │  [2 SO]      [4 NC]     [6 GND]          │  ← bottom row
  └───────────────────────── port opening ───┘

  Pin 1 VCC  → Teensy VIN          Pin 3 SI  → Teensy Pin 1 (PB1)
  Pin 2 SO   → Teensy Pin 2 (PB2)  Pin 5 SCK → Teensy Pin 0 (PB0)
  Pin 6 GND  → Teensy GND          Pin 4 NC  — do not connect
```

---

## How to Use

### 1 — Load the right cartridge
| Mode | Cartridge | LSDJ sync setting |
|------|-----------|-------------------|
| mGB | mGB flash cart | — |
| LSDJ Slave | LSDJ | `SYNC → SLAVE` |
| Nanoloop | Nanoloop | `sync → slave` |

### 2 — Switch modes
Send a **MIDI Program Change** on **channel 16**:

| PC value | Mode selected | LED blinks at startup |
|----------|---------------|-----------------------|
| `0` | mGB | **1 blink** |
| `1` | LSDJ Slave Sync | **2 blinks** |
| `2` | Nanoloop Sync | **3 blinks** |

Mode is saved to EEPROM — remembered after power-off.  
You can send PC from M8, a DAW, or any MIDI controller on ch 16.

### 3 — Connect MIDI
**TRS jack (hardware MIDI):** plug M8 headphone/MIDI output → 3.5 mm TRS socket on GB.  
**USB MIDI:** connect USB cable to Teensy — appears as a MIDI device on PC/Mac.  
Both inputs work simultaneously.

### 4 — Power on
Turn on the DMG. The LED blinks to confirm the current mode, then goes into active state.

---

## Mode Details

### Mode 0 — mGB
Full MIDI support across all 5 Game Boy sound channels.

| MIDI Channel | GB Channel |
|-------------|------------|
| 1 | Pulse 1 (PU1) |
| 2 | Pulse 2 (PU2) |
| 3 | Wave (WAV) |
| 4 | Noise (NOI) |
| 5 | Poly (all channels) |

- Note On/Off, pitch, velocity → GB sound hardware
- Channel mapping can be reconfigured via EEPROM (Max editor)

### Mode 1 — LSDJ Slave Sync
Arduinoboy acts as clock master for LSDJ running in Slave mode.

| MIDI Note | Action |
|-----------|--------|
| C-2 (48) | Sequencer Start |
| C#2 (49) | Sequencer Stop |
| D-2 (50) | Normal tempo |
| D#2 (51) | ½ tempo |
| E-2 (52) | ¼ tempo |
| F-2 (53) | ⅛ tempo |
| Notes > 53 | Set LSDJ song row offset on Start |

MIDI clock (24 ppqn) drives LSDJ. Start/Stop/Continue supported.

### Mode 2 — Nanoloop Sync
MIDI clock (24 ppqn) is converted to Nanoloop sync pulses.  
Set Nanoloop to `sync: slave` before use.

---

## LED Signals

| LED behaviour | Meaning |
|---------------|---------|
| 1 blink at power-on | Mode: **mGB** |
| 2 blinks at power-on | Mode: **LSDJ Slave** |
| 3 blinks at power-on | Mode: **Nanoloop** |
| Blinks during use | MIDI clock/data activity |

---

## Flashing Firmware

**Requirements:** [PlatformIO](https://platformio.org) (recommended) or Arduino IDE + [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html).

**⚠️ Remove GB batteries (or disconnect link port VIN wire) before connecting USB.**

### PlatformIO
```sh
# From project root:
pio run --target upload
```

### Arduino IDE
1. Install Teensyduino addon
2. Board: `Teensy 2.0`
3. USB Type: `Serial + MIDI`
4. Open `Arduinoboy/Arduinoboy.ino` → Upload

### platformio.ini (already configured)
```ini
[env:teensy20]
platform  = teensy
board     = teensy20
framework = arduino
lib_deps  = MIDI Library
```

---

## File Structure

```
Arduinoboy/
├── Arduinoboy.ino          — main setup/loop, EEPROM, platform defines
├── Mode.ino                — mode dispatcher
├── Mode_MidiGb.ino         — mGB implementation
├── Mode_LSDJ_SlaveSync.ino — LSDJ Slave sync
├── Mode_Nanoloop.ino       — Nanoloop sync
├── Led_Functions.ino       — LED blink helpers
├── Memory_Functions.ino    — EEPROM read/write
└── UsbMidi.ino             — USB MIDI (Teensyduino)

Instruction.md              — full wiring, pinouts, components (EN)
Instruction.ru.md           — то же на русском
Editor/
└── ArduinoBoyEditor-Midi.maxpat  — Max patch for EEPROM settings
```

---

## Differences from Original Arduinoboy

| Feature | Original | S_TN fork |
|---------|----------|-----------|
| Target MCU | Arduino Uno/Nano | **Teensy 2.0** |
| MIDI IN | DIN-5 via 6N138 | **3.5 mm TRS Type A** (M8 compatible) |
| USB MIDI | No | **Yes** (Teensyduino) |
| Active modes | 7 | **3** (mGB, LSDJ Slave, Nanoloop) |
| Mode switching | Physical button | **MIDI PC ch 16** |
| LEDs | 6 (one per mode) | **1** (onboard Teensy LED) |
| Build target | External module | **Inside DMG shell** |
| Power | USB or external | **GB link port pin 1** |
| Build system | Arduino IDE only | **PlatformIO + Arduino IDE** |

---

## Credits

- [trash80](https://github.com/trash80) — original Arduinoboy & mGB
- [PJRC](https://www.pjrc.com/teensy/) — Teensy 2.0 hardware & Teensyduino
- [GWEM](http://www.preromanbritain.com/gwem/lsdj_midi/g33k.html) — LSDJ MIDI research
- [devrs.com/gb](http://devrs.com/gb) — DMG link port serial specs
- [chipmusic.org](http://chipmusic.org)
