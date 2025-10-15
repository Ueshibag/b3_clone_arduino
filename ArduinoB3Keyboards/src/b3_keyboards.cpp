#include "b3_keyboards.h"
#include <Arduino.h>
#include <avr/sleep.h>

/******************************************************************
              SetBfree Keyboards Control Firmware
                      Arduino Nano Every.
 ******************************************************************/

// snapshot value of all keyboards switches (KEYBOARDS_NB_PINS)
static unsigned long old_value_g[MATRIX_NB_COLS];
static byte note_on_sent_g[KEYBOARDS_NB_PINS / 8];
static byte note_off_sent_g[KEYBOARDS_NB_PINS / 8];

// Debounce state machine variables
#define DEBOUNCE_TIME 2  // ms
enum DebounceState { STABLE_LOW, DEBOUNCING_LOW, STABLE_HIGH, DEBOUNCING_HIGH };
DebounceState debounce_states[KEYBOARDS_NB_PINS];
unsigned long debounce_times[KEYBOARDS_NB_PINS];

// MIDI buffer variables
#define MIDI_BUFFER_SIZE 32
byte midi_buffer[MIDI_BUFFER_SIZE][3];
byte buffer_head = 0;
byte buffer_tail = 0;


// USB keep-alive variables
unsigned long last_keepalive_time = 0;
#define KEEPALIVE_INTERVAL 10000  // 10 seconds

void setup() {
    sleep_disable();
    setup_keyboards_ctrl_pins();
    init_keyboards();
    Serial.begin(115200);
}

void setup_keyboards_ctrl_pins(void) {

    // Set T0-T7 as OUTPUT and LOW
    for (int i = T0; i <= T7; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
    }
    // Set BRA, MKA, BRB, MKB as INPUT
    const int input_pins[] = {BRA, MKA, BRB, MKB};
    for (int i = 0; i < 4; i++) {
        pinMode(input_pins[i], INPUT);
    }
    // Set MUX_A1, MUX_A2, MUX_A3 as OUTPUT and LOW
    const int mux_pins[] = {MUX_A1, MUX_A2, MUX_A3};
    for (int i = 0; i < 3; i++) {
        pinMode(mux_pins[i], OUTPUT);
        digitalWrite(mux_pins[i], LOW);
    }
}

void init_keyboards(void) {
    for (int column = 0; column < MATRIX_NB_COLS; column++) {
        old_value_g[column] = 0x00000000;
    }
    for (int i = 0; i < KEYBOARDS_NB_PINS / 8; i++) {
        note_on_sent_g[i] = 0x00;
        note_off_sent_g[i] = 0x00;
    }
    // Initialize debounce states
    for (int i = 0; i < KEYBOARDS_NB_PINS; i++) {
        debounce_states[i] = STABLE_LOW;
        debounce_times[i] = 0;
    }
}

void loop() {
    static unsigned long time = 0;
    static unsigned int active_column = 0;
    if (time == 0)
        time = micros();
    unsigned long curTime = micros();
    if (curTime > time + 100) {  // 100µs loop period
        select_keyboard_column(active_column);
        byte switches[4];
        read_all_switches(switches);
        look_for_changes(switches, active_column);
        time = curTime;
        if (++active_column >= MATRIX_NB_COLS)
            active_column = 0;
    }
    // Send buffered MIDI messages
    while (buffer_tail != buffer_head) {
        Serial.write(midi_buffer[buffer_tail], 3);
        buffer_tail = (buffer_tail + 1) % MIDI_BUFFER_SIZE;
    }

    // Send dummy MIDI message every 10 seconds
    if (millis() - last_keepalive_time >= KEEPALIVE_INTERVAL) {
        send_keepalive();
        last_keepalive_time = millis();
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
    unsigned long new_value = values[0] & 0xFF;
    new_value |= ((unsigned long)values[1]) << 8;
    new_value |= ((unsigned long)values[2]) << 16;
    new_value |= ((unsigned long)values[3]) << 24;
    unsigned long changed = new_value ^ old_value_g[col];
    if (changed) {
        old_value_g[col] = new_value;
        unsigned long mask = 0x01;
        for (byte row = 0; row < MATRIX_NB_ROWS; row++, mask <<= 1) {
            if (changed & mask) {
                bool closed = (new_value & mask) ? CLOSED : OPEN;
                notify_toggle(row, col, closed);
            }
        }
    }
}

void notify_toggle(byte row, byte col, bool closed) {
    int key = 8 * (row / 2) + col;
    byte chnl = (key >= 64 ? UPPER : LOWER);
    byte brk = (row & 1);
    int pitch = (key % 64) + 36;
    if (pitch > 96) pitch = 96;
    else if (pitch < 0) pitch = 0;
    byte key_mask = (1 << (key % 8));
    byte* note_on_sent = (byte*)&note_on_sent_g[key / 8];
    byte* note_off_sent = (byte*)&note_off_sent_g[key / 8];

    // Debounce state machine
    unsigned long now = millis();
    bool debounced_state = false;

    switch (debounce_states[key]) {
        case STABLE_LOW:
            if (closed) {
                debounce_states[key] = DEBOUNCING_LOW;
                debounce_times[key] = now;
            }
            break;
        case DEBOUNCING_LOW:
            if (now - debounce_times[key] >= DEBOUNCE_TIME) {
                if (closed) {
                    debounce_states[key] = STABLE_HIGH;
                    debounced_state = true;
                } else {
                    debounce_states[key] = STABLE_LOW;
                }
            }
            break;
        case STABLE_HIGH:
            if (!closed) {
                debounce_states[key] = DEBOUNCING_HIGH;
                debounce_times[key] = now;
            }
            break;
        case DEBOUNCING_HIGH:
            if (now - debounce_times[key] >= DEBOUNCE_TIME) {
                if (!closed) {
                    debounce_states[key] = STABLE_LOW;
                    debounced_state = false;
                } else {
                    debounce_states[key] = STABLE_HIGH;
                }
            }
            break;
    }

    if (brk) {
        // Break switch: only used for debouncing
        if (closed) {
            *note_on_sent &= ~key_mask;
            *note_off_sent &= ~key_mask;
        }
        return;
    }

    // Only proceed if the debounced state is stable
    if (debounced_state || !closed) {
        if (closed) {
            if (!(*note_on_sent & key_mask)) {
                *note_on_sent |= key_mask;
                send_note(chnl, pitch, ON);
            }
        } else {
            if (!(*note_off_sent & key_mask)) {
                *note_off_sent |= key_mask;
                send_note(chnl, pitch, OFF);
                *note_on_sent &= ~key_mask;
            }
        }
    }
}

void send_note(byte chnl, byte pitch, bool on) {
    byte bytes[3];
    bytes[0] = on ? (NOTE_ON | chnl) : (NOTE_OFF | chnl);
    bytes[1] = pitch;
    bytes[2] = on ? VELOCITY_MAX : VELOCITY_MIN;

    // Buffer MIDI messages ---
    midi_buffer[buffer_head][0] = bytes[0];
    midi_buffer[buffer_head][1] = bytes[1];
    midi_buffer[buffer_head][2] = bytes[2];
    buffer_head = (buffer_head + 1) % MIDI_BUFFER_SIZE;
}


void send_keepalive(void) {
    byte bytes[3] = {0x80, 0x00, 0x00};  // Note Off for note 0, velocity 0 (dummy message)
    midi_buffer[buffer_head][0] = bytes[0];
    midi_buffer[buffer_head][1] = bytes[1];
    midi_buffer[buffer_head][2] = bytes[2];
    buffer_head = (buffer_head + 1) % MIDI_BUFFER_SIZE;
}
