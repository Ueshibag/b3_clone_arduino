/*
  B3Midi.h - Library for MIDI communication over USB with the B3 clone emulator.
*/
#ifndef B3MIDI_H_
#define B3MIDI_H_

#include "Arduino.h"

#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define VELOCITY_MIN 0
#define VELOCITY_MAX 0x7F

class B3Midi
{
    public:
        B3Midi();
        void sendControlChange(byte channel, byte control, byte value);
        void sendProgramChange(byte channel, byte program);
        void sendNoteOn(byte channel, byte pitch);
        void sendNoteOff(byte channel, byte pitch);
};

#endif

