#include "b3_keyboards.h"
#include <Arduino.h>

/******************************************************************
              SetBfree Keyboards Control Firmware
                      Arduino Nano Every.

  This B3 clone houses two Fatar keyboards.
  
  Each key contact is made of two switches: when a key is pressed
  the break switch is closed first, then the make switch closes.
  Measuring the delta timing between both events gives information
  about the key velocity. Although the velocity is not taken into
  account in the organ emulation, reading the state of both switches
  is used to implement debouncing.

  Pedals control is no more necessary since I have bought a pedalboard
  from PedaMidiKit which produces MIDI messages by its own.
 ******************************************************************/



// snapshot value of all keyboards switches (KEYBOARDS_NB_PINS)
static unsigned long old_value_g[MATRIX_NB_COLS];

static byte note_on_sent_g[KEYBOARDS_NB_PINS / 8];
static byte note_off_sent_g[KEYBOARDS_NB_PINS / 8];

void setup() {

    setup_keyboards_ctrl_pins();

    init_keyboards();

    Serial.begin(115200);
}


void setup_keyboards_ctrl_pins(void) {

    pinMode(T0, OUTPUT);
    pinMode(T1, OUTPUT);
    pinMode(T2, OUTPUT);
    pinMode(T3, OUTPUT);
    pinMode(T4, OUTPUT);
    pinMode(T5, OUTPUT);
    pinMode(T6, OUTPUT);
    pinMode(T7, OUTPUT);

    digitalWrite(T0, LOW);
    digitalWrite(T1, LOW);
    digitalWrite(T2, LOW);
    digitalWrite(T3, LOW);
    digitalWrite(T4, LOW);
    digitalWrite(T5, LOW);
    digitalWrite(T6, LOW);
    digitalWrite(T7, LOW);

    pinMode(BRA, INPUT);
    pinMode(MKA, INPUT);
    pinMode(BRB, INPUT);
    pinMode(MKB, INPUT);

    pinMode(MUX_A1, OUTPUT);
    pinMode(MUX_A2, OUTPUT);
    pinMode(MUX_A3, OUTPUT);

    digitalWrite(MUX_A1, LOW);
    digitalWrite(MUX_A2, LOW);
    digitalWrite(MUX_A3, LOW);
}


void init_keyboards(void) {

    for (int column = 0; column < MATRIX_NB_COLS; column++) {
        // default buttons state is: released
        old_value_g[column] = 0x00000000;
    }

    for (int i = 0; i < KEYBOARDS_NB_PINS / 8; i++) {
        note_on_sent_g[i] = 0x00;
        note_off_sent_g[i] = 0x00;
    }
}

void loop() {

    static unsigned long time = 0;
    static unsigned int active_column = 0;

    if (time == 0)
        time = micros();

    unsigned long curTime = micros();

    // if 100us have elapsed, select a new Fatar keyboard matrix column
    if (curTime > time + 100) {
        select_keyboard_column(active_column);

        // read all switches of both keyboards at a time
        byte switches[4];
        read_all_switches(switches);

        look_for_changes(switches, active_column);

        time = curTime;

        if (++active_column >= MATRIX_NB_COLS)
            active_column = 0;
    }
}


void select_keyboard_column(unsigned int column) {

    digitalWrite(T0, column == 0 ? HIGH : LOW);
    digitalWrite(T1, column == 1 ? HIGH : LOW);
    digitalWrite(T2, column == 2 ? HIGH : LOW);
    digitalWrite(T3, column == 3 ? HIGH : LOW);
    digitalWrite(T4, column == 4 ? HIGH : LOW);
    digitalWrite(T5, column == 5 ? HIGH : LOW);
    digitalWrite(T6, column == 6 ? HIGH : LOW);
    digitalWrite(T7, column == 7 ? HIGH : LOW);
}


void read_all_switches(byte* switches) {

    switches[0] = 0;
    switches[1] = 0;
    switches[2] = 0;
    switches[3] = 0;

    for (byte mux = 0; mux <= 7; mux++) {
        digitalWrite(MUX_A1, (mux & 1) == 0 ? LOW : HIGH);
        digitalWrite(MUX_A2, (mux & 2) == 0 ? LOW : HIGH);
        digitalWrite(MUX_A3, (mux & 4) == 0 ? LOW : HIGH);

        int bra = digitalRead(BRA);
        int mka = digitalRead(MKA);

        int brb = digitalRead(BRB);
        int mkb = digitalRead(MKB);

        if (mux <= 3) {
            switches[0] |= (mkb << (mux * 2));
            switches[0] |= (brb << ((mux * 2) + 1));
            switches[2] |= (mka << (mux * 2));
            switches[2] |= (bra << ((mux * 2) + 1));

        } else {
            switches[1] |= (mkb << ((mux - 4) * 2));
            switches[1] |= (brb << (((mux - 4) * 2) + 1));
            switches[3] |= (mka << ((mux - 4) * 2));
            switches[3] |= (bra << (((mux - 4) * 2) + 1));
        }
    }
}

void look_for_changes(byte* values, byte col) {

    // make a 32-bit word from 4 bytes
    unsigned long new_value = values[0] & 0xFF;
    new_value |= ((unsigned long)values[1]) << 8;
    new_value |= ((unsigned long)values[2]) << 16;
    new_value |= ((unsigned long)values[3]) << 24;

    unsigned long changed = new_value ^ old_value_g[col];

    if (changed) {
        // store new value
        old_value_g[col] = new_value;

        unsigned long mask = 0x01;

        for (byte row = 0; row < MATRIX_NB_ROWS; row++, mask <<= 1) {
            if (changed & mask)
                notify_toggle(row, col, (new_value & mask) ? CLOSED : OPEN);
        }
    }
}


void notify_toggle(byte row, byte col, bool closed) {

    // determine key number, with col[7:0] and row[31:0]
    // key = 127 to 64 for ukb, 63 to 0 for lkb
    int key = 8 * (row / 2) + col;

    // determine MIDI channel
    byte chnl = (key >= 64 ? UPPER : LOWER);

    // check if key is assigned to a break switch
    byte brk = (row & 1);  // odd numbers

    // determine pitch (note number of a 61-note keyboard)
    // 36 is the lowest C key value of a 5-octave keyboard (C1)
    // 96 is C6
    int pitch = (key % 64) + 36;

    // ensure valid pitch range
    if (pitch > 96)
        pitch = 96;
    else if (pitch < 0)
        pitch = 0;

    // determine key mask and pointers for access to combined arrays
    byte key_mask = (1 << (key % 8));

    byte* note_on_sent = (byte*)&note_on_sent_g[key / 8];
    byte* note_off_sent = (byte*)&note_off_sent_g[key / 8];

    // break contacts do not send MIDI notes, they release the Note On/Off debouncing mechanism
    if (brk) {
        if (closed) {
            *note_on_sent &= ~key_mask;
            *note_off_sent &= ~key_mask;
        }
        return;
    }

    // make switch depressed or released?
    if (closed) {

        if (!(*note_on_sent & key_mask)) {
            *note_on_sent |= key_mask;
            send_note(chnl, pitch, ON);
        }

    } else {

        if (!(*note_off_sent & key_mask)) {
            *note_off_sent |= key_mask;
            send_note(chnl, pitch, OFF);
        }
    }
}


void send_note(byte chnl, byte pitch, bool on) {

    byte bytes[3];
    bytes[0] = on ? (NOTE_ON | chnl) : (NOTE_OFF | chnl);
    bytes[1] = pitch;
    bytes[2] = on ? VELOCITY_MAX : VELOCITY_MIN;
    Serial.write(bytes, 3);
}
