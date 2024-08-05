#include "b3_drawbars.h"
#include <Arduino.h>


/*************************************************************************
	SetBfree Harmonic Drawbars Control.

  From bass to treble pipes, drawbars names are the following:
  Controller indexes are decimal values

  Controller    Function
	  70          drawbar16
	  71          drawbar513
	  72          drawbar8
	  73          drawbar4
	  74          drawbar223
	  75          drawbar2
	  76          drawbar135
	  77          drawbar113
	  78          drawbar1

  Drawbar position decimal values of the MIDI Control Change:
  8 = 00    (loudest)
  7 = 01-15
  6 = 16-31
  5 = 32-47
  4 = 48-63
  3 = 64-79
  2 = 80-92
  1 = 96-110
  0 = 111-127   (off)

  Examples:
  B0 70 00  = [Control Change Channel 0] [drawbar16] [drawbar position 8]
  B0 70 111 = [Control Change Channel 0] [drawbar16] [drawbar position 0]
  B0 72 80  = [Control Change Channel 0] [drawbar8]  [drawbar position 2]

 *************************************************************************

  idx    anlgIdx    mux
  =============================
	0          0       0       UPKB REG1 RV1
	1          0       1
	2          0       2
	3          0       3
	4          0       4
	5          0       5
	6          0       6
	7          0       7
	8          1       X       UPKB REG1 RV9

    9          2       0       UPKB REG2 RV1
   10          2       1
   11          2       2
   12          2       3
   13          2       4
   14          2       5
   15          2       6
   16          2       7
   17          3       0       UPKB REG2 RV9
   18          3       1       PDKB      RV10
   19          3       2       PDKB      RV11

   20          4       0       LOKB REG1 RV1
   21          4       1
   22          4       2
   23          4       3
   24          4       4
   25          4       5
   26          4       6
   27          4       7
   28          5       X       LOKB REG1 RV9

   29          6       0       LOKB REG2 RV1
   30          6       1
   31          6       2
   32          6       3
   33          6       4
   34          6       5
   35          6       6
   36          6       7
   37          7       X       LOKB REG2 RV9
*/

// multiplexers controls (see Drawbars Control Board KiCad schematics)
const int MUX_A = 2;  // D2
const int MUX_B = 3;  // D3
const int MUX_C = 4;  // D4

// selected registration LEDs controls
const int UP_REG_LED = 5;  // D5
const int LO_REG_LED = 6;  // D6

const int DEBUG_LED = 13;

// analog reading with average calculation
int analogPins[8] = { A0, A1, A2, A3, A4, A5, A6, A7 };

// we have NB_DRAWBARS drawbars and every one has 9 possible positions, which can be coded on one byte
// we send a MIDI message on initialization, or when a drawbar position has changed
byte pos_old[NB_DRAWBARS];
byte pos_new[NB_DRAWBARS];

//                    0    1   2   3   4   5   6   7   8
byte dbar_pos[9] = { 127, 110, 92, 79, 63, 47, 31, 15, 0 };

// allows resetting the Arduino programmatically on reception of RESET_CMD
void(* reset_func) (void) = 0;


/*
   Arduino program setup. This function is executed only once.
*/
void setup() {

    setup_ctrl_pins();

    analogReference(EXTERNAL);  // Vdd of the ATmega4809
    Serial.begin(115200);

    // the very first call to analogRead() after powering up returns junk;
    // this is a documented issue with the ATmega chips.
    analogRead(A0);

    for (int i = 0; i < NB_DRAWBARS; i++) {
        pos_old[i] = DRAWBAR_POS_INIT;
        pos_new[i] = DRAWBAR_POS_INIT;
    }

    // wait for Raspberry PI connections; any character received triggers
    while (! Serial.available())
        delay(DELAY_100_MS);
    
    String id = Serial.readStringUntil(NEW_LINE);

    if (id.equals(DRAWBARS_IDENTIFIER)) {
        toggle_registration_leds(2);
        send_drawbars_initial_position();
    }
}


void setup_ctrl_pins(void) {

    pinMode(MUX_A, OUTPUT);
    pinMode(MUX_B, OUTPUT);
    pinMode(MUX_C, OUTPUT);

    pinMode(UP_REG_LED, OUTPUT);
    pinMode(LO_REG_LED, OUTPUT);

    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
    pinMode(A2, INPUT);
    pinMode(A3, INPUT);
    pinMode(A4, INPUT);
    pinMode(A5, INPUT);
    pinMode(A6, INPUT);
    pinMode(A7, INPUT);

    // unused pins.
    pinMode(7, INPUT_PULLUP);
    pinMode(8, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(11, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);

    // for control purpose
    pinMode(DEBUG_LED, OUTPUT);
    digitalWrite(DEBUG_LED, LOW);

    // initialize multiplexer
    digitalWrite(MUX_A, LOW);
    digitalWrite(MUX_B, LOW);
    digitalWrite(MUX_C, LOW);

    // switch ON the A registration LEDs for both keyboards
    digitalWrite(UP_REG_LED, HIGH);
    digitalWrite(LO_REG_LED, HIGH);
}


void send_drawbars_initial_position() {

    send_drawbars_positions(rc.UPPER_A);
    send_drawbars_positions(rc.LOWER_A);
    send_drawbars_positions(rc.BASS);
}


/*
  Endless Arduino program main loop.
*/
void loop() {

    static const unsigned long REFRESH_INTERVAL = DELAY_10_MS;
	static unsigned long last_refresh_time = 0;

    if (millis() - last_refresh_time >= REFRESH_INTERVAL)	{

		last_refresh_time += REFRESH_INTERVAL;
    
        send_active_moved_drawbars_settings();
        
        if (Serial.available() > 0) {

            String cmd = Serial.readStringUntil(NEW_LINE);

            if (cmd.equals(RESET_CMD))
                reset_func();
            else
                send_user_requested_preset(cmd);
        }
    }
}


void send_user_requested_preset(String preset) {

    if (preset == rc.UPPER_A) {

        digitalWrite(UP_REG_LED, HIGH);
        reg_up_a = true;
        reg_up_b = false;
        send_drawbars_positions(preset);
    }

    else if (preset == rc.UPPER_B) {

        digitalWrite(UP_REG_LED, LOW);
        reg_up_a = false;
        reg_up_b = true;
        send_drawbars_positions(preset);
    }

    else if (preset == rc.LOWER_A) {

        digitalWrite(LO_REG_LED, HIGH);
        reg_lo_a = true;
        reg_lo_b = false;
        send_drawbars_positions(preset);
    }

    else if (preset == rc.LOWER_B) {
      
        digitalWrite(LO_REG_LED, LOW);
        reg_lo_a = false;
        reg_lo_b = true;
        send_drawbars_positions(preset);
    }

    else if (preset == rc.UPPER_0) {
        
        reg_up_a = false;
        reg_up_b = false;
        send_program_change(0, 7);
    }

    else if (preset == rc.UPPER_1) {
        
        reg_up_a = false;
        reg_up_b = false;
        send_program_change(0, 8);
    }

    else if (preset == rc.UPPER_2) {
        
        reg_up_a = false;
        reg_up_b = false;
        send_program_change(0, 9);
    }

    else if (preset == rc.UPPER_3) {
        
        reg_up_a = false;
        reg_up_b = false;
        send_program_change(0, 10);
    }

    else if (preset == rc.UPPER_4) {
        
        reg_up_a = false;
        reg_up_b = false;
        send_program_change(0, 11);
    }

    else if (preset == rc.UPPER_5) {
        
        reg_up_a = false;
        reg_up_b = false;
        send_program_change(0, 12);
    }

    else if (preset == rc.LOWER_0) {
        
        reg_lo_a = false;
        reg_lo_b = false;
        send_program_change(0, 1);
    }

    else if (preset == rc.LOWER_1) {
        
        reg_lo_a = false;
        reg_lo_b = false;
        send_program_change(0, 2);
    }

    else if (preset == rc.LOWER_2) {
        
        reg_lo_a = false;
        reg_lo_b = false;
        send_program_change(0, 3);
    }

    else if (preset == rc.LOWER_3) {
        
        reg_lo_a = false;
        reg_lo_b = false;
        send_program_change(0, 4);
    }

    else if (preset == rc.LOWER_4) {
        
        reg_lo_a = false;
        reg_lo_b = false;
        send_program_change(0, 5);
    }

    else if (preset == rc.LOWER_5) {
        
        reg_lo_a = false;
        reg_lo_b = false;
        send_program_change(0, 6);
    }
}


void select_drawbar(int mux) {

    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);
}


void send_drawbars_positions(String preset) {

    if (preset == rc.UPPER_A) {
        for (int idx = 0; idx <= 8; idx++) {
            on_drawbar_move(idx);
        }
    }

    else if (preset == rc.UPPER_B) {
        for (int idx = 9; idx <= 17; idx++) {
            on_drawbar_move(idx);
        }
    }

    else if (preset == rc.BASS) {
        for (int idx = 18; idx <= 19; idx++) {
            on_drawbar_move(idx);
        }
    }
    
    else if (preset == rc.LOWER_A) {
        for (int idx = 20; idx <= 28; idx++) {
            on_drawbar_move(idx);
        }
    }

    else if (preset == rc.LOWER_B) {
        for (int idx = 29; idx <= 37; idx++) {
            on_drawbar_move(idx);
        }
    }
}


void send_active_moved_drawbars_settings() {

    int idx = 0;  // drawbar index
    int anlgMeasure = 0;

    //                       -- UPPER --    BASS    --- LOWER ---
    // drawbars array index [0..8][9..17] [18..19] [20..28][29..37]

    // =================================  Board 1  ==================================

    for (int mux = 0; mux <= 7; mux++) {

        select_drawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[0]);
        store_drawbar_position(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            on_drawbar_move(idx);

        idx++;
    }

    delay(10);  // ms
    anlgMeasure = analogRead(analogPins[1]);
    store_drawbar_position(anlgMeasure, idx);

    if (pos_new[idx] != pos_old[idx])
        on_drawbar_move(idx);

    idx++;

    // =================================  Board 2  ==================================

    for (int mux = 0; mux <= 7; mux++) {

        select_drawbar(mux);
        delay(10);  //ms
        anlgMeasure = analogRead(analogPins[2]);
        store_drawbar_position(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            on_drawbar_move(idx);

        idx++;
    }

    for (int mux = 0; mux <= 2; mux++) {

        select_drawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[3]);
        store_drawbar_position(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            on_drawbar_move(idx);

        idx++;
    }

    // =================================  Board 3  ==================================
    
    for (int mux = 0; mux <= 7; mux++) {

        select_drawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[4]);
        store_drawbar_position(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            on_drawbar_move(idx);

        idx++;
    }

    delay(10);  // ms
    anlgMeasure = analogRead(analogPins[5]);
    store_drawbar_position(anlgMeasure, idx);

    if (pos_new[idx] != pos_old[idx])
        on_drawbar_move(idx);

    idx++;

    // =================================  Board 4  ==================================

    for (int mux = 0; mux <= 7; mux++) {

        select_drawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[6]);
        store_drawbar_position(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            on_drawbar_move(idx);

        idx++;
    }

    delay(10);  // ms
    anlgMeasure = analogRead(analogPins[7]);
    store_drawbar_position(anlgMeasure, idx);

    if (pos_new[idx] != pos_old[idx])
        on_drawbar_move(idx);
}


void store_drawbar_position(int anlgMeasure, int idx) {

    if (anlgMeasure >= 0 && anlgMeasure <= 63)
        pos_new[idx] = 0;

    else if (anlgMeasure >= 64 && anlgMeasure <= 191)
        pos_new[idx] = 1;

    else if (anlgMeasure >= 192 && anlgMeasure <= 319)
        pos_new[idx] = 2;

    else if (anlgMeasure >= 320 && anlgMeasure <= 447)
        pos_new[idx] = 3;

    else if (anlgMeasure >= 448 && anlgMeasure <= 575)
        pos_new[idx] = 4;

    else if (anlgMeasure >= 576 && anlgMeasure <= 703)
        pos_new[idx] = 5;

    else if (anlgMeasure >= 704 && anlgMeasure <= 831)
        pos_new[idx] = 6;

    else if (anlgMeasure >= 832 && anlgMeasure <= 959)
        pos_new[idx] = 7;

    else if (anlgMeasure >= 960 && anlgMeasure <= 1023)
        pos_new[idx] = 8;
}


void on_drawbar_move(int idx) {

    if (is_drawbar_index_in_user_registration_range(idx)) {

        byte midiChnl = get_midi_channel(idx);
        int midiCtrl = get_midi_controller(idx);
        send_control_change(midiChnl, midiCtrl, dbar_pos[pos_new[idx]]);
    }

    pos_old[idx] = pos_new[idx];
}


bool is_drawbar_index_in_user_registration_range(int idx) {

    return  (idx >= 0 && idx <= 8 && reg_up_a) ||
            (idx >= 9 && idx <= 17 && reg_up_b) ||
            (idx == 18 || idx == 19) ||
            (idx >= 20 && idx <= 28 && reg_lo_a) ||
            (idx >= 29 && idx <= 37 && reg_lo_b);
}


byte get_midi_channel(int idx) {

    if (idx >= 0 && idx <= 8)
        return mc.UPPER_A;

    else if (idx >= 9 && idx <= 17)
        return mc.UPPER_B;

    else if (idx == 18 || idx == 19)
        return mc.BASS;

    else if (idx >= 20 && idx <= 28)
        return mc.LOWER_A;

    else
        return mc.LOWER_B;
}


int get_midi_controller(int idx) {

    if (idx >= 0 && idx <= 17)
        return 70 + (idx % 9);

    else if (idx == 18)
        return 70;

    else if (idx == 19)
        return 72;

    else
        return 70 + ((idx - 20) % 9);
}


void send_control_change(byte channel, byte controller, byte value) {
    byte bytes[3];
    bytes[0] = 0xB0 | channel;
    bytes[1] = controller;
    bytes[2] = value;
    Serial.write(bytes, 3);
    
    // Keep these lines commented out for non audible drawbars moves.
    // digitalWrite(DEBUG_LED, HIGH);
    // delay(DELAY_100_MS * 2);
    // digitalWrite(DEBUG_LED, LOW);
}


void send_program_change(byte channel, byte program) {
    byte bytes[2];
    bytes[0] = 0xC0 | channel;
    bytes[1] = program;
    Serial.write(bytes, 2);

    // Keep these lines commented out for non audible drawbars moves.
    // digitalWrite(DEBUG_LED, HIGH);
    // delay(DELAY_100_MS * 2);
    // digitalWrite(DEBUG_LED, LOW);
    // delay(DELAY_100_MS * 2);
    // digitalWrite(DEBUG_LED, HIGH);
    // delay(DELAY_100_MS * 2);
    // digitalWrite(DEBUG_LED, LOW);
}


void toggle_registration_leds(int toggles)
{
    for (int t=0; t<toggles; t++) {
        digitalWrite(UP_REG_LED, HIGH);
        digitalWrite(LO_REG_LED, HIGH);
        delay(DELAY_100_MS * 2);
        digitalWrite(UP_REG_LED, LOW);
        digitalWrite(LO_REG_LED, LOW);
        delay(DELAY_100_MS * 2);
    }
}
