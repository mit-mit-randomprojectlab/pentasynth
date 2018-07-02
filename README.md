# pentasynth
Pentasynth is a homebuilt keyboard/synthesizer based on a pentatonic scale.

This version uses Arduino Timers 0, 1 and 2 and a combination of hardware 
PWM for the tones and a pseudo-random noise driven by an interupt on Timer0 
to produce square wave and random noise audio signals. All key and 
accompaniment outputs are also sent as MIDI messages via USB/serial (and
can be used to generate audio on a connected computer using a MIDI compatible
software synthesizer.

requires: Arduino MIDI Library (https://github.com/FortySevenEffects/arduino_midi_library/)

Hardware PWM code adapted from: TVout (https://playground.arduino.cc/Main/TVout)
and Timer3 (http://playground.arduino.cc/Code/Timer1)
