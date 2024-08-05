/*
 B3Midi.cpp - Library for MIDI communication over USB with the B3 clone emulator.
*/

#include "Arduino.h"
#include "B3Midi.h"

B3Midi::B3Midi()
{
    Serial.begin(115200);
}

void B3Midi::sendControlChange(byte channel, byte control, byte value)
{
    byte bytes[3];
    bytes[0] = 0xB0 | channel;
    bytes[1] = control;
    bytes[2] = value;
    Serial.write(bytes, 3);
}

void B3Midi::sendProgramChange(byte channel, byte program)
{
    byte bytes[2];
    bytes[0] = 0xC0 | channel;
    bytes[1] = program;
    Serial.write(bytes, 2);
}

void B3Midi::sendNoteOn(byte channel, byte pitch)
{
    byte bytes[3];
    bytes[0] = NOTE_ON | channel;
    bytes[1] = pitch;
    bytes[2] = VELOCITY_MAX;
    Serial.write(bytes, 3);
}

void B3Midi::sendNoteOff(byte channel, byte pitch)
{
    byte bytes[3];
    bytes[0] = NOTE_OFF | channel;
    bytes[1] = pitch;
    bytes[2] = VELOCITY_MIN;
    Serial.write(bytes, 3);
}

