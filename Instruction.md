# Arduinoboy S_TN — Wiring & Build Instructions
## Teensy 2.0 inside DMG Game Boy

---

## 1. Teensy 2.0 Pinout (Teensyduino)

Used pins marked with ★:

```
              ┌──────[USB]──────┐
         GND ─┤                 ├─ VCC
★ SCK  PB0 0 ─┤ → GB Link Pin 5 ├─ 21 PF0
★  SI  PB1 1 ─┤ → GB Link Pin 3 ├─ 20 PF1
★  SO  PB2 2 ─┤ ← GB Link Pin 2 ├─ 19 PF4
       PB3 3 ─┤                 ├─ 18 PF5
       PB7 4 ─┤                 ├─ 17 PF6
       PD0 5 ─┤  ATmega32U4     ├─ 16 PF7
       PD1 6 ─┤                 ├─ 15 PB6
★ MIDI PD2 7 ─┤ ← Serial1 RX   ├─ 14 PB5
       PD3 8 ─┤                 ├─ 13 PB4
       PC6 9 ─┤                 ├─ 12 PD7
      PC7 10 ─┤                 ├─★11 PD6 ← ONBOARD LED
              └─────────────────┘
                       ▲
               VIN ← GB Link Pin 1 (VCC +5V)
               GND ← GB Link Pin 6 (GND)
```

| Teensy Pin | Port | Function                | Connection        |
|------------|------|-------------------------|-------------------|
| **Pin 0**  | PB0  | SCK — clock out         | → GB Link Pin 5   |
| **Pin 1**  | PB1  | SI — data to GB         | → GB Link Pin 3   |
| **Pin 2**  | PB2  | SO — data from GB       | ← GB Link Pin 2   |
| **Pin 7**  | PD2  | MIDI IN (Serial1 RX)    | ← 6N138 output    |
| **Pin 11** | PD6  | Onboard LED             | —                 |
| **VIN**    | —    | 5V power in             | ← GB Link Pin 1   |
| **GND**    | —    | Ground                  | ← GB Link Pin 6   |

---

## 2. DMG Link Port → Teensy 2.0

### DMG EXT Connector — Pin Numbering

```
  FRONT SIDE (as seen from outside, how cable plugs in):

        left edge of GB            right edge of GB
        ┌───────────────────────────────────────┐
        │   2 (SO)    4 (NC)    6 (GND)         │  ← top row
        │   ────────────────────────────        │
        │   1 (VCC)   3 (SI)    5 (SCK)         │  ← bottom row
        └─────────────────────────────────── ╗  │
                                             ╚══╝  ← port opening
```

```
  SOLDER SIDE — as seen from inside GB when soldering:

        left edge of GB            right edge of GB
        ┌───────────────────────────────────────┐
        │   1 (VCC)   3 (SI)    5 (SCK)         │  ← top row    ★ SOLDER HERE
        │   ────────────────────────────        │     (divider/ridge)
        │   2 (SO)    4 (NC)    6 (GND)         │  ← bottom row ★ SOLDER HERE
        └─────────────────────────────────── ╗  │
                                             ╚══╝  ← port opening (bottom of GB)
```

Measured voltages on pads (GB powered on, idle):

```
  SOLDER SIDE:
  ┌────────────────────────────────────────────────────┐
  │  [1 / VCC]   [3 / SI]    [5 / SCK]                │
  │   ~4.85 V    ~5.09 V     ~5.09 V                  │  ← top row
  │  ─────────────────────────────────                 │
  │  [2 / SO]    [4 / NC]    [6 / GND]                │
  │    0 V         0 V         0 V                    │  ← bottom row
  └──────────────────────────────────── port opening ─┘
```

> VCC ~4.85V — battery voltage (normal).  
> SI and SCK ~5.09V — pulled up to VCC (5.09V on GB bus), idle-high.  
> SO = 0V — idle-low, carries data only during transmission.  
> ⚠️ Pins **3 (SI) and 5 (SCK)** are adjacent in the top row — avoid solder bridges!

**Connection table (solder side, left to right):**

| Position (solder side) | DMG Pin | Signal      | Teensy          | Direction             |
|------------------------|---------|-------------|-----------------|-----------------------|
| top left               | **1**   | **VCC +5V** | **VIN**         | GB → Teensy (power)   |
| top center             | **3**   | **SI**      | **Pin 1** (PB1) | Teensy → GB (data)    |
| top right              | **5**   | **SCK**     | **Pin 0** (PB0) | Teensy → GB (clock)   |
| bottom left            | **2**   | **SO**      | **Pin 2** (PB2) | GB → Teensy (data)    |
| bottom center          | **4**   | NC          | —               | do not connect        |
| bottom right           | **6**   | **GND**     | **GND**         | GB → Teensy (ground)  |

| Link Pin | Signal     | Direction             | Teensy 2.0      | Port         |
|----------|------------|-----------------------|-----------------|--------------|
| 1        | VCC (+5V)  | GB → Teensy (power)   | **VIN**         | —            |
| 2        | SO         | GB → Teensy (data)    | **Pin 2** (PB2) | PORTB bit 2  |
| 3        | SI         | Teensy → GB (data)    | **Pin 1** (PB1) | PORTB bit 1  |
| 4        | SD/NC      | not used              | —               | —            |
| 5        | SCK        | Teensy → GB (clock)   | **Pin 0** (PB0) | PORTB bit 0  |
| 6        | GND        | Common ground         | **GND**         | —            |

> Pins 0, 1, 2 share PORTB register — allows atomic clock+data manipulation with a single `PORTB = ...` instruction.

### Wiring Diagram

```
  DMG LINK PORT (solder side)          TEENSY 2.0
  ┌──────────────────────┐             ┌──────────────────────┐
  │ top-left   Pin 1 VCC ├────────────►│ VIN                  │
  │                      │             │                      │
  │ top-center Pin 3  SI │◄────────────│ Pin 1 (PB1)          │ → data TO GB
  │                      │             │                      │
  │ top-right  Pin 5 SCK │◄────────────│ Pin 0 (PB0)          │ → clock
  │                      │             │                      │
  │ bot-left   Pin 2  SO ├────────────►│ Pin 2 (PB2)          │ ← data FROM GB
  │                      │             │                      │
  │ bot-center Pin 4  NC │  (no conn)  │                      │
  │                      │             │                      │
  │ bot-right  Pin 6 GND ├────────────►│ GND                  │
  └──────────────────────┘             │                      │
                                       │ Pin 7 (PD2) ◄── MIDI IN
                                       │ Pin 11(PD6) ──► LED  │
                                       └──────────────────────┘
```

---

## 3. MIDI IN: 3.5mm TRS Type A (M8) → 6N138 → Teensy

### M8 TRS Type A MIDI Out

```
  TRS Jack
  ─ Tip    = DIN-5 Pin 5 (current sink / transistor M8)
  ─ Ring   = DIN-5 Pin 4 (source +V through 220Ω inside M8)
  ─ Sleeve = GND
```

### Optocoupler Circuit (6N138)

```
  TRS JACK          6N138 (DIP-8)                    TEENSY 2.0
  ─────────        ┌──────────────┐                  ──────────
                   │ 1 NC         │  do not connect
  Ring (+) ──220Ω──┤ 2 A (Anode)  │
  Tip  (−) ────────┤ 3 C (Cathode)│
  1N5819: cathode  │ 4 NC         │  do not connect
  (stripe)→Pin2,   │ 5 GND        ├─────────────────────── GND
  anode→Pin3       │ 6 V0 (Out)   ├──┬────────────────────── Pin 7 (RX)
                   │ 7 NC         │  │  do not connect
  VCC (+5V) ───────┤ 8 VCC   ←─270Ω──┘  (pull-up between Pin 6 and VCC)
                   └──────────────┘
  Pin 8 → VCC  direct (no resistor)
  270Ω  → between Pin 6 and VCC (pull-up)
  Pin 5 → GND  direct (no resistor to ground needed)

  Sleeve ─────────────────────────────────────────────── GND
```

> **Only 2 resistors needed:** 220Ω (input) + 270Ω (output pull-up)

### MIDI Circuit Components

| Component    | Value   | Purpose                              |
|--------------|---------|--------------------------------------|
| R1           | 220 Ω   | Current limiting resistor for LED    |
| R2           | 270 Ω   | Pull-up on collector output (PC900 style) |
| D1 (1N5819)  | —       | Reverse polarity protection          |
| IC1 (6N138)  | —       | Optocoupler / galvanic isolation      |

---

## 4. LED

```
  Onboard LED on Teensy 2.0: Pin 11 (PD6)
  No external LED required.

  To add external LED visible outside the GB:
  Pin 11 ──── 220Ω ──── LED(+) ──── LED(−) ──── GND
```

---

## 5. Full Wiring Diagram

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                        ARDUINOBOY S_TN                                         │
│                     (inside DMG Game Boy)                                      │
│                                                                                │
│   DMG LINK PORT (solder side)                                                  │
│   ─────────────────────────                                                    │
│   top-left  Pin 1 VCC  ──────────────────────────► VIN        ┐               │
│   top-center Pin 3  SI ◄─────────────────────────  Pin 1(PB1) │               │
│   top-right Pin 5 SCK  ◄─────────────────────────  Pin 0(PB0) │ TEENSY 2.0    │
│   bot-left  Pin 2  SO  ──────────────────────────► Pin 2(PB2) │               │
│   bot-center Pin 4  NC  (no conn)                             │               │
│   bot-right Pin 6 GND  ──────────────────────────── GND       │               │
│                                                    Pin 11(PD6)── ONBOARD LED  │
│                                                    Pin 7 (PD2)◄─ MIDI IN RX   │
│                                                               ┘               │
│                                               ▲                                │
│                               ┌───────────────┘                                │
│   3.5mm TRS JACK (M8 MIDI Out)    6N138 OPTOISOLATOR                           │
│   ──────────────────────────     ┌─────────────────┐                          │
│   Ring (+) ──220Ω──────────────► │ Anode(2)        │                          │
│   Tip  (−) ─────────────────── ► │ Cathode(3)      │                          │
│   1N5819 (protection: Ring→Tip)  │ GND(5) ─── GND  │                          │
│   Sleeve ──────────────── GND    │ VCC(8) ─── VCC  │                          │
│                                  │ Out(6) ─270Ω─VCC│                          │
│                                  │ Out(6) ─────────┼────────────► Pin 7 (RX)  │
│                                  └─────────────────┘                          │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Modes & Switching (Reference)

| Mode       | Program Change | LED blink at power-on | M8 channel                |
|------------|----------------|------------------------|---------------------------|
| mGB        | PC 0           | **1 blink**            | MIDI Out → mGB ch 1-5     |
| LSDJ Slave | PC 1           | **2 blinks**           | Clock/Transport → GB LSDJ |
| Nanoloop   | PC 2           | **3 blinks**           | Clock → GB Nanoloop       |

**Mode switching:** MIDI Program Change on **channel 16** (USB or TRS jack).

---

## 7. Components, Specifications & Alternatives

### 7.1 Required Components

| # | Component             | Qty    | Specifications                                        | Purpose                               |
|---|-----------------------|--------|-------------------------------------------------------|---------------------------------------|
| 1 | **Teensy 2.0**        | 1      | ATmega32U4, 5 V, 16 MHz, USB HID/MIDI, 25 GPIO, DIP  | Main microcontroller board            |
| 2 | **6N138**             | 1      | CTR 300–600 %, Vf 1.2 V, trise/tfall ≈15 µs, DIP-8   | MIDI IN optocoupler                   |
| 3 | **1N5819**            | 1      | Schottky, 40 V, 1 A, Vf ≈0.3 V, DO-41                | Reverse polarity protection (MIDI)    |
| 4 | **Resistor 220 Ω**    | 1      | 1/4 W, ±5 %, axial                                   | Current limit for 6N138 LED           |
| 5 | **Resistor 270 Ω**    | 1      | 1/4 W, ±5 %, axial                                   | Pull-up on 6N138 collector output     |
| 6 | **3.5 mm TRS socket** | 1      | Stereo, PCB mount, e.g. PJ-302M or equivalent         | MIDI IN (TRS Type A) from M8          |
| 7 | **Wire**              | ≈30 cm | 6 conductors, AWG 28–30, thin flexible                | DMG link port → Teensy                |

---

### 7.2 Alternatives — Teensy 2.0

Code targets **PORTB bits 0-2** of Teensy 2.0 — other boards require pin remapping in `Mode_MidiGb.ino` etc.

| Option                | MCU           | 5V | USB MIDI | Notes                                               |
|-----------------------|---------------|----|----------|-----------------------------------------------------|
| **Teensy 2.0** ✅     | ATmega32U4    | ✅ | ✅       | Recommended, code written for this board            |
| Arduino Pro Micro 5V  | ATmega32U4    | ✅ | ✅       | Same MCU, more compact, different pinout            |
| Arduino Leonardo      | ATmega32U4    | ✅ | ✅       | Same MCU, larger form factor                        |
| Arduino Micro         | ATmega32U4    | ✅ | ✅       | Same MCU, Pro Micro form factor                     |
| Teensy 2.0++          | AT90USB1286   | ✅ | ✅       | More flash/RAM, different pinout                    |
| Teensy LC             | MK20DX256     | ⚠️ | ✅       | 3.3 V only — needs level-shifters for GB 5V logic   |

---

### 7.3 Alternatives — Optocoupler (6N138)

| Option         | CTR      | Speed        | Extra pin  | Notes                                                   |
|----------------|----------|--------------|------------|---------------------------------------------------------|
| **6N138** ✅   | 300–600 %| ≈15 µs       | Pin 7 (base enable) | Recommended. Circuit as shown in this doc       |
| **6N137** ✅   | ≥19 %    | 10 Mbit/s    | None       | Faster, different circuit: no Pin 7, pull-up 330–470 Ω |
| **PC900V** ✅  | ≥100 %   | ≈1 Mbit/s    | None       | Original MIDI DIN-5 spec recommendation                |
| **H11L1** ✅   | ≥50 %    | ≈1.6 Mbit/s  | None       | Built-in Schmitt trigger — clean edges                  |
| **TLP2361** ✅ | —        | 15 Mbit/s    | None       | Modern replacement, logic-level output                  |
| **4N25** ⚠️   | ≥20 %    | ≈50 µs       | Pin 6 (base) | Too slow, may miss bits at MIDI 31.25 kbaud            |
| **PC817** ⚠️  | 50–300 % | ≈5 µs        | None       | No Schmitt trigger, risk of false triggering            |

---

### 7.4 Alternatives — Diode (1N5819)

| Option         | Type     | Vf (nom.) | Imax   | Notes                                        |
|----------------|----------|-----------|--------|----------------------------------------------|
| **1N5819** ✅  | Schottky | ≈0.30 V   | 1 A    | Recommended. Low Vf, fast                    |
| **1N4148** ✅  | Silicon  | ≈0.70 V   | 200 mA | Works. Ubiquitous, slightly higher Vf         |
| **BAT42** ✅   | Schottky | ≈0.35 V   | 200 mA | Direct 1N5819 equivalent, smaller body       |
| **BAT43** ✅   | Schottky | ≈0.35 V   | 200 mA | Equivalent to BAT42                          |
| **BAT85** ✅   | Schottky | ≈0.30 V   | 200 mA | Equivalent, available in SMD                 |
| **SS14** ✅    | Schottky | ≈0.40 V   | 1 A    | SMD (DO-214AC), 1N5819 equivalent            |

---

### 7.5 Alternatives — 3.5 mm TRS Socket

| Model               | Mount      | Manufacturer  | Notes                          |
|---------------------|------------|---------------|--------------------------------|
| **PJ-302M** ✅      | DIP        | —             | Most common, inexpensive       |
| **PJ-3001A**        | DIP        | —             | PJ-302M equivalent             |
| **SJ1-3535NG**      | DIP        | Switchcraft   | Reliable, quality build        |
| **FC68131**         | DIP        | Cliff UK      | Professional grade             |
| **CUI SJ-3523-SMT** | SMD        | CUI           | Surface mount                  |
| **WQP-PJ301M-12**   | Panel-mount | Qingpu       | For panel/enclosure mounting   |

---

### 7.6 Optional / Recommended Extras

| Component           | Value    | Qty | Purpose                                                    |
|---------------------|----------|-----|------------------------------------------------------------|
| Ceramic capacitor   | 100 nF   | 1–2 | Decoupling — place between VCC and GND close to Teensy     |
| Electrolytic cap.   | 10 µF    | 1   | Additional power filtering                                 |
| Heat shrink tubing  | 2–3 mm   | —   | Insulate solder joints on link port wires                  |
| Double-sided tape / glue | —   | —   | Secure Teensy inside GB shell                              |

---

*Powered from DMG battery (+5V, link port pin 1). USB is used for firmware flashing only.*

