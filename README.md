
# B3 clone Arduino projects

## Controls (Vibrato / Chorus / Leslie - Percussions / Expression Pedal)

Although these functionalities are spread over two boards, they are all controlled
by a single Arduino Nano Every.

## Drawbars

The 38 drawbars span over 4 boards ( 9 + 11 + 9 + 9) connected by wires.
The leftmost board houses an Arduino Nano Every which sends MIDI Control Change
messages to the Raspberry PI each time a drawbar is pushed or pulled.

## Keyboards

An Arduino Nano Every reacts to every keyboard note ON/OFF event by sending
the proper MIDI Note On/Off message to the B3 emulator.

