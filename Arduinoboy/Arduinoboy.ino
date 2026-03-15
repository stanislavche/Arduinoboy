/***************************************************************************
 * Arduinoboy — S_TN fork
 * Target:  Teensy 2.0 (ATmega32U4 + Teensyduino)
 * Modes:   0 = mGB  |  1 = LSDJ Slave Sync  |  2 = Nanoloop Slave
 * Switch:  MIDI Program Change on channel 16 (M8)
 * LED:     Onboard Teensy 2.0 LED (pin 11 / PD6)
 *            – on mode switch: 1 blink = mGB, 2 = LSDJ, 3 = Nanoloop
 * Power:   DMG Game Boy link port pin 1 (5 V)
 ***************************************************************************/

#define NUMBER_OF_MODES     3   // 0=mGB  1=LSDJ Slave  2=Nanoloop
#define MODE_SWITCH_CH     15   // MIDI channel 16 (0-indexed) — used for PC mode switch
#define MEM_MAX            65

// ── EEPROM address map ────────────────────────────────────────────────────
#define MEM_CHECK              0
#define MEM_VERSION_FIRST      1
#define MEM_VERSION_SECOND     2
#define MEM_MODE               5
#define MEM_FORCE_MODE         4
#define MEM_LSDJSLAVE_MIDI_CH  6
#define MEM_MGB_CH            55
#define MEM_MIDIOUT_BIT_DELAY  61
#define MEM_MIDIOUT_BYTE_DELAY 63

// ── Default settings (written to EEPROM on first boot) ───────────────────
byte defaultMemoryMap[MEM_MAX] = {
  0x7F,0x01,0x03,0x7F, // memory init check
  0x00,                // MEM_FORCE_MODE
  0x00,                // MEM_MODE  — default: mGB (0)
  15,                  // MEM_LSDJSLAVE_MIDI_CH  (ch 16, 0-indexed)
  15,15,1,1,           // unused legacy bytes (master ch, kbd ch, compat)
  1,                   // unused
  0,1,2,3,             // midiout note channels (unused)
  0,1,2,3,             // midiout CC channels (unused)
  1,1,1,1,             // midiout CC mode (unused)
  1,1,1,1,             // midiout CC scaling (unused)
  1,2,3,7,10,11,12,    // pu1 CC numbers (unused)
  1,2,3,7,10,11,12,    // pu2
  1,2,3,7,10,11,12,    // wav
  1,2,3,7,10,11,12,    // noi
  0,1,2,3,4,           // MEM_MGB_CH: mGB midi channels (ch 1-5, 0-indexed)
  0,                   // livemap ch (unused)
  80,1,                // midiout bit delay & multiplier (unused)
  0,0                  // midiout byte delay & multiplier (unused)
};
byte memory[MEM_MAX];

// ── Platform: Teensy 2.0 (ATmega32U4 + Teensyduino) ─────────────────────
#if defined(__AVR_ATmega32U4__) && defined(TEENSYDUINO)
  #define USE_TEENSY 1
  // GB link uses Port B bits 0,1,2 — atomic write in one instruction
  #define GB_SET(bit_cl, bit_out, bit_in) \
    PORTB = (PINB & ~0x07) | (((uint8_t)(bit_in))<<2 | ((uint8_t)(bit_out))<<1 | ((uint8_t)(bit_cl)))
  int pinGBClock     = 0;   // PB0 → DMG link pin 5 (SCK)
  int pinGBSerialOut = 1;   // PB1 → DMG link pin 3 (SI — data to GB)
  int pinGBSerialIn  = 2;   // PB2 → DMG link pin 2 (SO — data from GB)
  int pinStatusLed   = 11;  // PD6 — onboard LED on Teensy 2.0
  HardwareSerial *serial = &Serial1; // RX=pin 7 (PD2), TX=pin 8 (PD3)

// ── Platform: Teensy 3.x / LC (fallback) ─────────────────────────────────
#elif defined(__MK20DX256__) || defined(__MK20DX128__) || defined(__MKL26Z64__)
  #define USE_TEENSY 1
  #include <MIDI.h>
  #if defined(__MKL26Z64__)
    #define GB_SET(bit_cl,bit_out,bit_in) GPIOB_PDOR = ((bit_in<<3)|(bit_out<<1)|bit_cl)
  #else
    #define GB_SET(bit_cl,bit_out,bit_in) GPIOB_PDOR = (GPIOB_PDIR & 0xfffffff4)|((bit_in<<3)|(bit_out<<1)|bit_cl)
  #endif
  int pinGBClock     = 16;
  int pinGBSerialOut = 17;
  int pinGBSerialIn  = 18;
  int pinStatusLed   = 13;
  HardwareSerial *serial = &Serial1;

// ── Platform: Arduino generic fallback ───────────────────────────────────
#else
  #define GB_SET(bit_cl,bit_out,bit_in) PORTC = (PINC & B11111000)|((bit_in<<2)|(bit_out<<1)|bit_cl)
  int pinGBClock     = A0;
  int pinGBSerialOut = A1;
  int pinGBSerialIn  = A2;
  int pinStatusLed   = 13;
  HardwareSerial *serial = &Serial;
#endif

// ── EEPROM ────────────────────────────────────────────────────────────────
#include <EEPROM.h>
boolean alwaysUseDefaultSettings = false;

// ── State ─────────────────────────────────────────────────────────────────
boolean sequencerStarted    = false;
boolean midiSyncEffectsTime = false;
boolean midiNoteOnMode      = false;

// Mode-switch flags
boolean midiPcWait         = false;  // awaiting PC data byte on hardware MIDI ch16
boolean modeChangeRequested = false; // set by handleModeChange() → breaks mode while(1)

boolean statusLedIsOn  = false;
boolean statusLedBlink = false;

boolean nanoState    = false;
boolean nanoSkipSync = false;

// Single-LED blink management (replaces the original 6-element arrays)
boolean       blinkSwitch    = false;
unsigned long blinkSwitchTime = 0;
uint16_t      blinkMaxCount  = 1000;

// Sync counters
int     countLSDJTicks    = 0;
int     countSyncTime     = 0;
int     countSyncLightTime= 0;
int     countSyncSteps    = 0;
int     countSyncPulse    = 0;
int     countStatusLedOn  = 0;
uint8_t switchLight       = 0;

// MIDI parsing
byte incomingMidiByte;
byte midiData[]     = {0, 0, 0};
byte lastMidiData[] = {0, 0, 0};
int  lastMode       = 0;
byte midiStatusType;
byte midiStatusChannel;
boolean midiValueMode   = false;
boolean midiAddressMode = false;
byte    midiDefaultStartOffset = 0;
int     midioutBitDelay  = 0;
int     midioutByteDelay = 0;

#define GB_MIDI_DELAY 500  // µs between bytes sent to Game Boy

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
  initMemory(0);

  pinMode(pinGBClock,     OUTPUT);
  pinMode(pinGBSerialIn,  INPUT);
  pinMode(pinGBSerialOut, OUTPUT);
  pinMode(pinStatusLed,   OUTPUT);

  serial->begin(31250);

  digitalWrite(pinGBClock,     HIGH); // GB expects idle HIGH
  digitalWrite(pinGBSerialOut, LOW);

  usbMidiInit();
  startupSequence();
  showSelectedMode();
}

// ── Main loop ─────────────────────────────────────────────────────────────
void loop() {
  switchMode();
}
