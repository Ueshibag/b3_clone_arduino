#include "b3_controls.h"
#include <Arduino.h>


/*************************************************************************
 This is the firmware running on the Arduino Nano Every connected to the
 "Vibrato Chorus & Leslie Controls" board.
 
 The commands sent by this Arduino firmware can be checked by running setBfreeUI.
 Tests are launched by pressing the Overdrive push button at least 3 seconds.

 Detection of state/level changes on (the ON/OFF state of switches marked
 with * is signaled by a LED.):

 Vibrato-Chorus switches group :
 - overdrive *
 - vibrato upper *
 - vibrato lower *
 - vibrato-chorus rotary switch

 Leslie :
 - stop
 - slow
 - fast

 Percussion switches group :
 - percussion on/off *
 - percussion volume *
 - percussion delay *
 - percussion harmonic *

 Expression pedal
 ______________________________________________________________________________

 IMPORTANT NOTE read from setbfree default.pgm file !!!
 # Note that by default the program-number is in the range 1..128.
 # The first program (display-number: "1") is loaded by sending a
 # MIDI patch-change to activate program "0". This can be changed by setting
 # pgm.controller.offset=0 in the configuration.

 Therefore, default.cfg should contain the following:
 ## MIDI controller program number offset
 ## The value of this parameter should be equal to the number of the
 ## lowest program number on your MIDI controller. Some use 0 others 1.
 ##
 pgm.controller.offset=0
 ______________________________________________________________________________
*/

// --------------------------- pins assignments -------------------------------

const int OVERDRIVE_SWITCH = 0;      // D0
const int OVERDRIVE_LED = 1;         // D1
const int VIBRATO_UPPER_SWITCH = 2;  // D2
const int VIBRATO_UPPER_LED = 3;     // D3
const int VIBRATO_LOWER_SWITCH = 4;  // D4
const int VIBRATO_LOWER_LED = 5;     // D5

const int V1 = 6;   // D6
const int C1 = 7;   // D7
const int V2 = 8;   // D8
const int C2 = 9;   // D9
const int V3 = 10;  // D10
const int C3 = 11;  // D11

const int PERC_ON_OFF_LED = 12;       // D12
const int PERC_ON_OFF_SWITCH = 13;    // D13
const int PERC_VOLUME_LED = 21;       // D21
const int PERC_VOLUME_SWITCH = 20;    // D20
const int PERC_DELAY_LED = 19;        // D19
const int PERC_DELAY_SWITCH = 18;     // D18
const int PERC_HARM_SEL_SWITCH = 17;  // D17
const int PERC_HARM_LED = 16;         // D16
const int EXPR_PEDAL = A1;
const int LESLIE = A0;


// Allows resetting the Arduino programmatically on reception of RESET_CMD.
void (*reset_func)(void) = 0;


#ifdef PIO_UNIT_TESTING
SerialMock Serialm;
#endif


/*
  Called when a sketch starts. Initializes variables, pin modes, start using libraries, etc.
  The setup() function will only run once, after each power up or reset of the Arduino board.
  Use PIO_UNIT_TESTING guard to avoid collision with test code setup() function.
*/
#ifndef PIO_UNIT_TESTING
void setup() {

    setup_ctrl_pins();

    analogReference(EXTERNAL);  // Vdd of the ATmega4809
    Serial.begin(115200);

    // the very first call to analogRead() after power up returns junk;
    // this is a documented issue with the ATmega chips.
    analogRead(LESLIE);

    // wait for Raspberry PI connections
    // any character received from the RPI exits the loop
    while (!Serial.available())
        delay(DELAY_100_MS);

    String id = Serial.readStringUntil(NEW_LINE);

    if (id.equals(CONTROLS_IDENTIFIER)) {

        set_controls_initial_state();
        toggle_leds(2);
    }
}
#endif

void setup_ctrl_pins(void) {

    // switches
    pinMode(OVERDRIVE_SWITCH, INPUT);
    pinMode(VIBRATO_UPPER_SWITCH, INPUT);
    pinMode(VIBRATO_LOWER_SWITCH, INPUT);
    pinMode(V1, INPUT_PULLUP);
    pinMode(C1, INPUT_PULLUP);
    pinMode(V2, INPUT_PULLUP);
    pinMode(C2, INPUT_PULLUP);
    pinMode(V3, INPUT_PULLUP);
    pinMode(C3, INPUT_PULLUP);
    pinMode(PERC_ON_OFF_SWITCH, INPUT);
    pinMode(PERC_VOLUME_SWITCH, INPUT);
    pinMode(PERC_DELAY_SWITCH, INPUT);
    pinMode(PERC_HARM_SEL_SWITCH, INPUT);

    // LEDs
    pinMode(OVERDRIVE_LED, OUTPUT);
    pinMode(VIBRATO_UPPER_LED, OUTPUT);
    pinMode(VIBRATO_LOWER_LED, OUTPUT);
    pinMode(PERC_ON_OFF_LED, OUTPUT);
    pinMode(PERC_VOLUME_LED, OUTPUT);
    pinMode(PERC_DELAY_LED, OUTPUT);
    pinMode(PERC_HARM_LED, OUTPUT);

    // analog inputs
    pinMode(LESLIE, INPUT);
    pinMode(EXPR_PEDAL, INPUT);
}

void set_controls_initial_state() {

    set_overdrive(OFF);
    set_vibrato_upper(OFF);
    set_vibrato_lower(OFF);

    on_vibrato_chorus_change(V1, VIBRATO_V1);
    on_vibrato_chorus_change(C1, VIBRATO_C1);
    on_vibrato_chorus_change(V2, VIBRATO_V2);
    on_vibrato_chorus_change(C2, VIBRATO_C2);
    on_vibrato_chorus_change(V3, VIBRATO_V3);
    on_vibrato_chorus_change(C3, VIBRATO_C3);

    set_percussion(OFF);
    set_percussion_volume(NORMAL);
    set_percussion_delay(SLOW);
    set_percussion_harmonic(SECOND);

    on_leslie_change();
    on_expression_pedal_change();
}

/*
 * Software reset occurs when we get the RESET_CMD string of chars.
*/
void on_reset()
{
    if (Serial.available() == 2) {
        String cmd = Serial.readStringUntil(NEW_LINE);
        if (cmd.equals(RESET_CMD))
            reset_func();
    }
}

/*
  Endless Arduino program main loop.
  Use PIO_UNIT_TESTING guard to avoid collision with test code loop() function.
*/
#ifndef PIO_UNIT_TESTING
void loop() {

    static const unsigned long REFRESH_INTERVAL_MS = 10;
    static unsigned long last_refresh_time = 0;

    if (millis() - last_refresh_time >= REFRESH_INTERVAL_MS) {

        last_refresh_time += REFRESH_INTERVAL_MS;

        on_control_change(OVERDRIVE_SWITCH, set_overdrive);
        on_control_change(VIBRATO_UPPER_SWITCH, set_vibrato_upper);
        on_control_change(VIBRATO_LOWER_SWITCH, set_vibrato_lower);

        on_vibrato_chorus_change(V1, VIBRATO_V1);
        on_vibrato_chorus_change(C1, VIBRATO_C1);
        on_vibrato_chorus_change(V2, VIBRATO_V2);
        on_vibrato_chorus_change(C2, VIBRATO_C2);
        on_vibrato_chorus_change(V3, VIBRATO_V3);
        on_vibrato_chorus_change(C3, VIBRATO_C3);

        on_control_change(PERC_ON_OFF_SWITCH, set_percussion);
        on_control_change(PERC_VOLUME_SWITCH, set_percussion_volume);
        on_control_change(PERC_DELAY_SWITCH, set_percussion_delay);
        on_control_change(PERC_HARM_SEL_SWITCH, set_percussion_harmonic);

        on_leslie_change();
        on_expression_pedal_change();

        on_reset();
    }
}
#endif // PIO_UNIT_TESTING



void on_control_change(int b3_switch, void (*set_b3_control)(bool)) {

    // Control pins are in the D0-D20 range
    constexpr int ARRAY_SIZE = 21;

    static bool s_old[ARRAY_SIZE];
    static bool s_new[ARRAY_SIZE];
    static bool s_on[ARRAY_SIZE];

    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            s_old[i] = LOW;
            s_new[i] = LOW;
            s_on[i] = false;
        }
        initialized = true;
    }

#ifndef PIO_UNIT_TESTING
    s_new[b3_switch] = digitalRead(b3_switch);
#else
    s_new[b3_switch] = !s_new[b3_switch]; // mock button pressed/released
#endif

    if (s_new[b3_switch] != s_old[b3_switch]) {
        if (s_new[b3_switch] == LOW) {
            s_on[b3_switch] = !s_on[b3_switch];
            set_b3_control(s_on[b3_switch]);
        }
        s_old[b3_switch] = s_new[b3_switch];
    }
}

void set_overdrive(bool on) {
    digitalWrite(OVERDRIVE_LED, on);
    on ? send_program_change(OVERDRIVE_ON) : send_program_change(OVERDRIVE_OFF);
}

void set_vibrato_upper(bool on) {
    digitalWrite(VIBRATO_UPPER_LED, on);
    on ? send_program_change(VIBRATO_UPPER_ON) : send_program_change(VIBRATO_UPPER_OFF);
}

void set_vibrato_lower(bool on) {
    digitalWrite(VIBRATO_LOWER_LED, on);
    on ? send_program_change(VIBRATO_LOWER_ON) : send_program_change(VIBRATO_LOWER_OFF);
}

void on_vibrato_chorus_change(int vc_pin, int vc_prog) {
    
    // Since V1 to C3 are 6 consecutive integer values, we can use V1 as offset to index array elements.
    static int s_old[] = {-1, -1, -1, -1, -1, -1};
    static int s_new[] = {-1, -1, -1, -1, -1, -1};

#ifndef PIO_UNIT_TESTING
    s_new[vc_pin - V1] = digitalRead(vc_pin - V1);
#else
    s_new[vc_pin - V1] = !s_new[vc_pin - V1]; // mock button pressed/released
#endif

    if (s_new[vc_pin - V1] != s_old[vc_pin - V1]) {
        if (s_new[vc_pin - V1] == LOW)
            send_program_change(vc_prog);
        s_old[vc_pin - V1] = s_new[vc_pin - V1];
    }
}

void set_percussion(bool on) {
    digitalWrite(PERC_ON_OFF_LED, on);
    on ? send_program_change(PERCUSSION_ON) : send_program_change(PERCUSSION_OFF);
}

void set_percussion_volume(bool soft) {
    digitalWrite(PERC_VOLUME_LED, soft);
    soft ? send_program_change(PERCUSSION_VOLUME_SOFT) : send_program_change(PERCUSSION_VOLUME_NORMAL);
}

void set_percussion_delay(bool fast) {
    digitalWrite(PERC_DELAY_LED, fast);
    fast ? send_program_change(PERCUSSION_DELAY_FAST) : send_program_change(PERCUSSION_DELAY_SLOW);
}

void set_percussion_harmonic(bool third) {
    digitalWrite(PERC_HARM_LED, third);
    third ? send_program_change(PERCUSSION_HARMONIC_3) : send_program_change(PERCUSSION_HARMONIC_2);
}

void on_leslie_change() {

    static byte leslie_old = CTRL_INIT;
    static byte leslie_new = CTRL_INIT;

    int anlgMeasure = analogRead(LESLIE);
    leslie_new = get_leslie_position(anlgMeasure);

    if (leslie_new != leslie_old) {

        send_program_change(leslie_new);
        leslie_old = leslie_new;
    }
}

byte get_leslie_position(int anlgMeasure) {

    byte position = LESLIE_STOP;

    if (anlgMeasure >= 0 && anlgMeasure < 50)
        position = LESLIE_SLOW;

    else if (anlgMeasure > 990 && anlgMeasure < 1030)
        position = LESLIE_FAST;

    return position;
}

void on_expression_pedal_change() {

    static byte expr_pedal_old;
    static byte expr_pedal_new;

    int anlgMeasure = analogRead(EXPR_PEDAL);
    expr_pedal_new = get_expr_pedal_position(anlgMeasure);

    if (expr_pedal_new != expr_pedal_old) {

        send_control_change(UPPER_MIDI_CHNL, VOLUME_CONTROL, expr_pedal_new);
        send_control_change(LOWER_MIDI_CHNL, VOLUME_CONTROL, expr_pedal_new);
        send_control_change(PEDAL_MIDI_CHNL, VOLUME_CONTROL, expr_pedal_new);

        expr_pedal_old = expr_pedal_new;
    }
}

byte get_expr_pedal_position(int anlgMeasure) {

    byte position = anlgMeasure / 8;
    return position;
}

void send_control_change(byte channel, byte control, byte value) {

    byte bytes[3];
    bytes[0] = 0xB0 | channel;
    bytes[1] = control;
    bytes[2] = value;
#ifdef PIO_UNIT_TESTING
    Serialm.write(bytes, 3);
#else
    Serial.write(bytes, 3);
#endif
}

void send_program_change(byte program) {

    byte bytes[2];
    bytes[0] = 0xC0 | UPPER_MIDI_CHNL;
    bytes[1] = program;
#ifdef PIO_UNIT_TESTING
    Serialm.write(bytes, 2);
#else
    Serial.write(bytes, 2);
#endif
}

void toggle_leds(int nb_toggles, int led_idx = -1) {

    const int NB_LEDS = 7;
    const int leds[NB_LEDS] = {OVERDRIVE_LED, VIBRATO_UPPER_LED, VIBRATO_LOWER_LED, PERC_ON_OFF_LED, PERC_VOLUME_LED, PERC_DELAY_LED, PERC_HARM_LED};

    for (int t = 0; t < 2 * nb_toggles; t++) {

        if (led_idx == -1) {
            // toggle ALL LEDs
            for (int d = 0; d < NB_LEDS; d++)
                t % 2 ? digitalWrite(leds[d], OFF) : digitalWrite(leds[d], ON);
        }
        else {
            // toggle a SINGLE LED
            t % 2 ? digitalWrite(leds[led_idx], OFF) : digitalWrite(leds[led_idx], ON);
        }

        delay(DELAY_100_MS * 5);
    }
}
