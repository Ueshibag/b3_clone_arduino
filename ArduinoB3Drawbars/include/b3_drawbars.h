// ===========================================================================
// b3_drawbars.h
// includes other project header files
// ===========================================================================
#ifndef B3_DRAWBARS_H
#define B3_DRAWBARS_H

#include <Arduino.h>

// a value out of the possible values range, used to initialize pos_old and pos_new arrays
// so that whatever the first drawbar position measurement is, we will send a CC message
#define DRAWBAR_POS_INIT 255

#define NB_DRAWBARS 38  // 4 x 9 + 2

// sent by Raspberry PI to identify the Drawbars board
#define DRAWBARS_IDENTIFIER "D"

#define DELAY_10_MS 10
#define DELAY_100_MS 100

// sent by RPI to reset the Arduino
#define RESET_CMD "R"

#define NEW_LINE '\n'

// used as MIDI channel value in Control Change messages
struct midiChannel {
    byte UPPER_A = 0;
    byte UPPER_B = 0;
    byte LOWER_A = 1;
    byte LOWER_B = 1;
    byte BASS = 2;
};
midiChannel mc;


// sent by RPI to Arduino to change drawbars settings
struct rpiCommand {

    char UPPER_0[3] = "U0";
    char UPPER_1[3] = "U1";
    char UPPER_2[3] = "U2";
    char UPPER_3[3] = "U3";
    char UPPER_4[3] = "U4";
    char UPPER_5[3] = "U5";
    char UPPER_A[3] = "UA";
    char UPPER_B[3] = "UB";
    char LOWER_0[3] = "L0";
    char LOWER_1[3] = "L1";
    char LOWER_2[3] = "L2";
    char LOWER_3[3] = "L3";
    char LOWER_4[3] = "L4";
    char LOWER_5[3] = "L5";
    char LOWER_A[3] = "LA";
    char LOWER_B[3] = "LB";
    char BASS[2] = "B";
    byte ETB = 23;  // ASCII End of Transmission Block
};
rpiCommand rc;


// active registrations flags
bool reg_up_a = true;
bool reg_up_b = false;
bool reg_lo_a = true;
bool reg_lo_b = false;

/*
* Sets pins mode (input, output, pull-up...)
*/
void setup_ctrl_pins(void);


/*
* On reception of the DRAWBARS_IDENTIFIER character from the Raspberry PI
* /usr/bin/runb3 script, sends the 'A' drawbars set positions to setBfree
* for both keyboards. Bass drawbars positions are sent as well.
* On completion of this function, the organ sounds should reflect the
* Upper A, Lower A and Bass drawbars positions.
*/
void send_drawbars_initial_position(void);


/*
* Drawbars positions have to be sent to setBfree on initialization as well as
* on registration change.
* On initialization: upper A, lower A and bass drawbars settings are sent.
* On registration change: only settings of drawbars matching the selected
* registration are sent.
*
* @param preset: preset sent by the RPI
*/
void send_drawbars_positions(String preset);


/*
  Stores a drawbar position (0..8). It will be compared to the
  previous position to check if the drawbar has been moved.

	anlgMeasure:    analog value measured on the drawbar
	idx:            index of the drawbar [0..37]
*/
void store_drawbar_position(int anlgMeasure, int idx);


/*
* Reads active drawbars positions and reacts to changes by sending MIDI Control Change
* messages to the corresponding MIDI controller.
* A drawbar is active if it is part of the selected preset (A or B).
*/
void send_active_moved_drawbars_settings();


/*
* Uses a demultiplexer to select a drawbar for position reading.
*
* @param mux: multiplex value in [0..7] range
*/
void select_drawbar(int mux);


/*
* Prepares drawbars MIDI Control Change data, then sends the message to setBfree.
* MIDI CC messages are sent if and only if the drawbar index matches the selected
* keyboard registration (excepted for bass).
*
* @param idx: index of the moving drawbar in the [0..37] range
*/
void on_drawbar_move(int idx);


/*
* The organ player can change registration settings any time.
* Upper [0..5] and Lower [0..5] are fixed presets declared as setBfree MIDI
* programs into $HOME/.config/setBfree/default.pgm. When these presets
* are selected, we send MIDI Program Change to setBfree.
* Upper [A, B] and Lower [A, B] are user defined presets; when they are
* selected, we send MIDI Control Change messages to setBfree.
*
* @param preset: preset requested by user (one of Upper [A,B, 0..5], Lower [A,B, 0..5])
*/
void send_user_requested_preset(String preset);


/*
  Moving a drawbar has an effect on the organ sound if:
  - the selected registration is a user preset (A or B)
  and
  - the drawbar index is in the range of the selected user preset.
  Bass (pedals) drawbars are always active.
*/
bool is_drawbar_index_in_user_registration_range(int idx);


/*
  Sends a MIDI Program Change message over the USB link.
  A PC message is sent whenever a registration preset button is pressed.
*/
void send_program_change(byte channel, byte program);


/*
  Upper keyboard drawbars moves produce MIDI CC messages on channel 0.
  Lower keyboard drawbars moves produce MIDI CC messages on channel 1.
  Pedal board drawbars moves produce MIDI CC messages on channel 2.
	
	idx:  index of the moving drawbar
*/
byte get_midi_channel(int idx);


/*
  Calculates and returns a drawbar MIDI controller index.

	idx:  index of the drawbar [0-37]
*/
int get_midi_controller(int idx);


/*
  Sends a MIDI CC message.
  A CC message is sent to setBfree whenever an active drawbar is moved.

	channel:    MIDI channel index
	controller: MIDI controller value
	value:      value byte to be sent as CC
*/
void send_control_change(byte channel, byte controller, byte value);


/*
* Toggles the A/B drawbars group selection LEDs indicators.
* @param toggles - number of toggles
*/
void toggle_registration_leds(int toggles);

#endif // B3_DRAWBARS_H
