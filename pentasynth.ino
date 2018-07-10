////////////////////////////////////////////////////////////////////
// pentasynth: An arduino-based pentatonic MIDI keyboard/synthesizer
//
// Code by mit-mit (http://randomprojectlab.blogspot.com.au/)
//
// Pentasynth is a homebuilt keyboard/synthesizer based on a pentatonic scale.
// 
// This version uses Arduino Timers 0, 1 and 2 and a combination of hardware 
// PWM for the tones and a pseudo-random noise driven by an interupt on Timer0 
// to produce square wave and random noise audio signals. All key and 
// accompaniment outputs are also sent as MIDI messages via USB/serial (and
// can be used to generate audio on a connected computer using a MIDI compatible
// software synthesizer.
// 
// requires: Arduino MIDI Library (https://github.com/FortySevenEffects/arduino_midi_library/)
//
// Hardware PWM code adapted from: TVout (https://playground.arduino.cc/Main/TVout)
// and Timer3 (http://playground.arduino.cc/Code/Timer1)
//
 
#include <MIDI.h>

#include "drumsandbass.h"
#include "pitches.h"

// 
#define DDR_SND DDRB
#define PORT_SND PORTB // Port B is pins 8-13 on the Uno
#define SND_PIN 3 // Pin 11
#define SND_PIN2 1 // Pin 9
#define SND_PIN3 5 // Pin 13 (on-board LED flashes with beat :) )

// variables for tracking durations notes are held
volatile uint8_t noise_cycles_remain = 0;
volatile unsigned long tone1_cycles_remain = 0;
volatile unsigned long tone2_cycles_remain = 0;

volatile byte current_bass_note = 0;
volatile byte bass_note_on = 0;
MIDI_CREATE_DEFAULT_INSTANCE(); // start up MIDI

// tonepwm: create audio with hardware PWM on Timer2
void tonepwm(unsigned int frequency, unsigned long duration_ms) {

  if (frequency == 0)
    return;
  
  #define TIMER 2

  if (duration_ms > 0)
    tone1_cycles_remain = duration_ms;
  else
    tone1_cycles_remain = 0;
  
  // Initialise
  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2A |= _BV(WGM21);
  TCCR2B |= _BV(CS20);

  // most of this is taken from Tone.cpp from Arduino
  uint8_t prescalarbits = 0b001;
  uint32_t ocr = 0;
  
  DDR_SND |= _BV(SND_PIN); //set pb3 (digital pin 11) to output

  //we are using an 8 bit timer, scan through prescalars to find the best fit
  ocr = F_CPU / frequency / 2 - 1;
  prescalarbits = 0b001;  // ck/1: same for both timers
  if (ocr > 255) {
        ocr = F_CPU / frequency / 2 / 8 - 1;
        prescalarbits = 0b010;  // ck/8: same for both timers

        if (ocr > 255) {
      ocr = F_CPU / frequency / 2 / 32 - 1;
      prescalarbits = 0b011;
        }

        if (ocr > 255) {
      ocr = F_CPU / frequency / 2 / 64 - 1;
      prescalarbits = TIMER == 0 ? 0b011 : 0b100;
      if (ocr > 255) {
        ocr = F_CPU / frequency / 2 / 128 - 1;
        prescalarbits = 0b101;
      }

      if (ocr > 255) {
        ocr = F_CPU / frequency / 2 / 256 - 1;
        prescalarbits = TIMER == 0 ? 0b100 : 0b110;
        if (ocr > 255) {
          // can't do any better than /1024
          ocr = F_CPU / frequency / 2 / 1024 - 1;
          prescalarbits = TIMER == 0 ? 0b101 : 0b111;
        }
      }
        }
    }
  TCCR2B = prescalarbits;
 
  // Set the OCR for the given timer,
  OCR2A = ocr;
  
  // set it to toggle the pin by itself
  TCCR2A &= ~(_BV(COM2A1)); //set COM2A1 to 0
  TCCR2A |= _BV(COM2A0);
  
} // end of tone

// Stops tone generation
void notonepwm() {
  TCCR2B = 0;
  PORT_SND &= ~(_BV(SND_PIN)); //set pin 11 to 0
  tone1_cycles_remain = 0;
} 

// tonepwm2: create audio with hardware PWM on Timer1
void tonepwm2(unsigned int frequency, unsigned long duration_ms) {

  int duty = 512;
  if (frequency == 0)
    return;

  if (duration_ms > 0)
    tone2_cycles_remain = duration_ms;
  else
    tone2_cycles_remain = 0;
  
  uint32_t ocr = 0;
  unsigned char clockSelectBits;
  unsigned long dutyCycle;

  // Initialise
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B = _BV(WGM13);
  
  // modified from TimerThree
  ocr = F_CPU / frequency / 2 - 1;
  if (ocr < 0xffff)              clockSelectBits = _BV(CS10);              // no prescale, full xtal
    else if((ocr >>= 3) < 0xffff) clockSelectBits = _BV(CS11);              // prescale by /8
    else if((ocr >>= 3) < 0xffff) clockSelectBits = _BV(CS11) | _BV(CS10);  // prescale by /64
    else if((ocr >>= 2) < 0xffff) clockSelectBits = _BV(CS12);              // prescale by /256
    else if((ocr >>= 2) < 0xffff) clockSelectBits = _BV(CS12) | _BV(CS10);  // prescale by /1024
    else        ocr = 0xffff - 1, clockSelectBits = _BV(CS12) | _BV(CS10);  // request was out of bounds, set as maximum
    ICR1 = ocr;                                                     // ICR1 is TOP in p & f correct pwm mode
    TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
    TCCR1B |= clockSelectBits;  
  
  // from TimerThree
  DDRB |= _BV(SND_PIN2);
  TCCR1A |= _BV(COM1A1);
  dutyCycle = ocr;
  dutyCycle *= duty;
  dutyCycle >>= 10;
  OCR1A = dutyCycle;
  TCCR1B |= clockSelectBits;
  
} // end of tonepwm2

// Stops tone generation (second channel)
void notonepwm2() {
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12)); // stop the tone
  PORTB &= ~(_BV(SND_PIN2)); // pin 9 on uno
  tone2_cycles_remain = 0;
} // end of notonepwm2


// noisegen: create pseudo-random noise audio (used for drum beat)
volatile unsigned long int lfsr = 1;

void noisegen(unsigned int frequency) {

  // Initialise interupt
  pinMode(13, OUTPUT); // drum output
  noise_cycles_remain = 20; // number of 1kHz cycles to play noise
  lfsr = 1;
  
} // end of noisegen

// Stops noise generation
void nonoisegen() {
  //TIMSK0 &= ~(1 << OCIE0A);
  PORT_SND &= ~(_BV(SND_PIN3));
} // end of nonoisegen

// Galois Linear Feedback Shift Register
// see: https://arduino.stackexchange.com/questions/6715/audio-frequency-white-noise-generation-using-arduino-mini-pro
#define LFSR_MASK  ((unsigned long)( 1UL<<31 | 1UL <<15 | 1UL <<2 | 1UL <<1  ))

// Interupt handler: controls drum beat psuedo random noise and tracks notes
// held by the accompaniment
ISR(TIMER0_COMPA_vect) 
{
  if (tone1_cycles_remain > 0) {
    tone1_cycles_remain--;
  }
  else {
    notonepwm();
  }
  if (tone2_cycles_remain > 0) {
    tone2_cycles_remain--;
  }
  else {
    notonepwm2();
    if (bass_note_on > 0) {
      MIDI.sendNoteOff(current_bass_note, 127, 2);
      bass_note_on = 0;
    }
  }
  if (noise_cycles_remain == 0) {
    nonoisegen();
  }
  else {
    noise_cycles_remain--;
    if (lfsr & 1) { 
      lfsr = (lfsr >>1) ^ LFSR_MASK;
      PORT_SND |= _BV(SND_PIN3);
    }
    else { 
      lfsr >>= 1;
      PORT_SND &= ~_BV(SND_PIN3); 
    }
  }
}

// Keyboard controls
uint8_t pins[] = {2,4,5,6,7,8,3,10,19,12}; // arduino pins for each key 
int pinval[] = {25,27,29,32,34,37,39,41,44,46, // major pentatonic
                25,28,30,32,35,37,40,42,44,47, // minor pentatonic
                25,27,30,32,34,37,39,42,44,46 // blues pentatonic
                }; // pitch offsets for each key
uint8_t pinlast[] = {0,0,0,0,0,0,0,0,0,0}; // stores key press state

// control pins
#define DRUMSWITCH_PIN 15
#define BASSSWITCH_PIN 16
#define CHORDSWITCH_PIN 17
#define MODESWITCH_PIN 18

uint8_t controlpinlast[] = {0,0,0,0};

// accompany controls
uint8_t drum_pattern = 0;
uint8_t bass_pattern = 0;
uint8_t chord_prog = 0;
uint8_t mode_type = 0;

uint8_t drum_index = 1;
uint8_t bass_index = 0;
uint8_t chord_index = 0;
uint8_t accomp_count = 0;
uint8_t accomp_beat = 0;
int accomp_delay = 10;

int tempo_val = 0;

void setup() 
{

  // Setup input key pins
  for (int i = 0; i < 10; i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
  pinMode(DRUMSWITCH_PIN, INPUT_PULLUP);
  pinMode(BASSSWITCH_PIN, INPUT_PULLUP);
  pinMode(CHORDSWITCH_PIN, INPUT_PULLUP);
  pinMode(MODESWITCH_PIN, INPUT_PULLUP);

  // Setup MIDI comms
  MIDI.begin();
  Serial.begin(115200);
  
  pinMode(13, OUTPUT); // noise generation output pin
  
  // setup interupt for counting tone durations (linked into default Timer 0 1kHz)
  OCR0A = 0x01;
  TIMSK0 |= _BV(OCIE0A);

  // setup default instruments
  MIDI.sendProgramChange(81,1);
  MIDI.sendProgramChange(1,2);
  
}

void loop() 
{
  // read keys, play main voice
  for (uint8_t i = 0; i < 10; i++) {
    if ( (digitalRead(pins[i]) == 0) && (pinlast[i] == 0) ) {
      pinlast[i] = 1;
      tonepwm(pitches[pinval[i+10*mode_type]+12],100000);
      MIDI.sendNoteOn(pinval[i+10*mode_type]+48-25, 127, 1);
      
    }
    else if ( (digitalRead(pins[i]) == 1) && (pinlast[i] == 1) ) {
      pinlast[i] = 0;
      notonepwm();
      MIDI.sendNoteOff(pinval[i+10*mode_type]+48-25, 127, 1);
    }
    else if (digitalRead(pins[i]) == 1) {
      pinlast[i] = 0;
    }
  }

  //////////////////////////////////////////////
  // Read in changes in drum, bass, chord etc.

  // Drums
  if ( (digitalRead(DRUMSWITCH_PIN) == 0) && (controlpinlast[0] == 0) ) {
    controlpinlast[0] = 1;
    drum_pattern++;
    if (drum_pattern == drum_pattern_n) {
      drum_pattern = 0;
    }
    drum_index = 0;
    while (drum_durations[drum_index+drum_n[drum_pattern]] < accomp_beat) {
      drum_index++;
      if (drum_index == (drum_n[drum_pattern+1]-drum_n[drum_pattern])) {
        drum_index = 0;
        break;
      }
    }
  }
  else if ( (digitalRead(DRUMSWITCH_PIN) == 1) && (controlpinlast[0] == 1) ) {
    controlpinlast[0] = 0;
  }
  else if (digitalRead(DRUMSWITCH_PIN) == 1) {
    controlpinlast[0] = 0;
  }

  // Bass
  if ( (digitalRead(BASSSWITCH_PIN) == 0) && (controlpinlast[1] == 0) ) {
    controlpinlast[1] = 1;
    bass_pattern++;
    if (bass_pattern == bass_pattern_n) {
      bass_pattern = 0;
    }
    bass_index = 0;
    while (bass_durations[bass_index+bass_n[bass_pattern]] < accomp_beat) {
      bass_index++;
      if (bass_index == (bass_n[bass_pattern+1]-bass_n[bass_pattern])) {
        bass_index = 0;
        break;
      }
    }
    /*drum_index = 0;
    bass_index = 0;
    chord_index = 0;
    accomp_count = 0;
    accomp_beat = 0;*/
  }
  else if ( (digitalRead(BASSSWITCH_PIN) == 1) && (controlpinlast[1] == 1) ) {
    controlpinlast[1] = 0;
    notonepwm2();
  }
  else if (digitalRead(BASSSWITCH_PIN) == 1) {
    controlpinlast[1] = 0;
  }

  // Chord progression
  if ( (digitalRead(CHORDSWITCH_PIN) == 0) && (controlpinlast[2] == 0) ) {
    controlpinlast[2] = 1;
    chord_prog++;
    if (chord_prog == chord_prog_n) {
      chord_prog = 0;
    }
    /*drum_index = 0;
    bass_index = 0;
    chord_index = 0;
    accomp_count = 0;
    accomp_beat = 0;*/
  }
  else if ( (digitalRead(CHORDSWITCH_PIN) == 1) && (controlpinlast[2] == 1) ) {
    controlpinlast[2] = 0;
  }
  else if (digitalRead(CHORDSWITCH_PIN) == 1) {
    controlpinlast[2] = 0;
  }

  // Pentatonic Mode
  if ( (digitalRead(MODESWITCH_PIN) == 0) && (controlpinlast[3] == 0) ) {
    controlpinlast[3] = 1;
    mode_type++;
    if (mode_type == 2) {
      mode_type = 0;
    }
  }
  else if ( (digitalRead(MODESWITCH_PIN) == 1) && (controlpinlast[3] == 1) ) {
    controlpinlast[3] = 0;
  }
  else if (digitalRead(MODESWITCH_PIN) == 1) {
    controlpinlast[3] = 0;
  }
  
  // Run loop for drum and bass
  if (drum_pattern > 0) {
    if (accomp_beat == drum_durations[drum_index+drum_n[drum_pattern]]) {
      noisegen(100);
      MIDI.sendNoteOn(drum_notes[drum_index+drum_n[drum_pattern]], 127, 10);
      MIDI.sendNoteOff(drum_notes[drum_index+drum_n[drum_pattern]], 0, 10);
      drum_index++;
      if (drum_index == (drum_n[drum_pattern+1]-drum_n[drum_pattern])) {
        drum_index = 0;
      }
    }
  }
  if (bass_pattern > 0) {
    if (accomp_beat == bass_durations[bass_index+bass_n[bass_pattern]]) { // playing on second voice channel
      tonepwm2(pitches[bass_notes[bass_index+bass_n[bass_pattern]]+25+chord_progressions[4*chord_prog+chord_index]], 100);
      current_bass_note = bass_notes[bass_index+bass_n[bass_pattern]]+48+chord_progressions[4*chord_prog+chord_index];
      MIDI.sendNoteOn(current_bass_note, 127, 2);
      bass_note_on = 1;
      bass_index++;
      if (bass_index == (bass_n[bass_pattern+1]-bass_n[bass_pattern])) {
        bass_index = 0;
      }
    }
  }

  // read in for changes in tempo
  tempo_val = 20 - 2*(analogRead(0)/100);
  if (tempo_val <= 1) {
    tempo_val = 2;
  }
  
  // control tempo
  accomp_count++;
  if (accomp_count >= tempo_val) {
    accomp_count = 0;
    accomp_beat++;
    if (accomp_beat == 32) {
      accomp_beat = 0;
      chord_index++;
      if (chord_index == 4) {
        chord_index = 0;
      }
    }
  }
  delay(accomp_delay);

}

