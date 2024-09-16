// ===========================================================================
// b3_controls.h
// includes other project header files
// ===========================================================================
#ifndef B3_CONTROLS_H
#define B3_CONTROLS_H

#include "b3_percussion.h"
#include "b3_vibrato_chorus.h"
#include <Arduino.h>

#define CTRL_INIT 127

#define UPPER_MIDI_CHNL 0
#define LOWER_MIDI_CHNL 1
#define PEDAL_MIDI_CHNL 2

#define VOLUME_CONTROL 7  // expression pedal controller

// ------------- MIDI Program values found by running 'setbfree -d' -----------

#define OVERDRIVE_OFF 40
#define OVERDRIVE_ON 41

#define LESLIE_STOP 52
#define LESLIE_SLOW 53
#define LESLIE_FAST 54

// sent by Raspberry PI to identify the Controls board
#define CONTROLS_IDENTIFIER "C"

// sent by Raspberry PI to perform last actions before shutting down
#define SHUTDOWN_CMD "S"

#define DELAY_100_MS 100

// sent by RPI to reset the Arduino
#define RESET_CMD "R"

#define NEW_LINE '\n'

// --------------------------- pins assignments -------------------------------

extern const int OVERDRIVE_SWITCH;
extern const int OVERDRIVE_LED;
extern const int VIBRATO_UPPER_SWITCH;
extern const int VIBRATO_UPPER_LED;
extern const int VIBRATO_LOWER_SWITCH;
extern const int VIBRATO_LOWER_LED;

extern const int PERC_ON_OFF_LED;
extern const int PERC_ON_OFF_SWITCH;
extern const int PERC_VOLUME_LED;
extern const int PERC_VOLUME_SWITCH;
extern const int PERC_DELAY_LED;
extern const int PERC_DELAY_SWITCH;
extern const int PERC_HARM_SEL_SWITCH;
extern const int PERC_HARM_LED;
extern const int EXPR_PEDAL;
extern const int LESLIE;


/*
* Sets pins mode (input, output, pull-up...)
*/
void setup_ctrl_pins(void);

/*
* When the organ is turned on, we want setBfree parameters to be set to initial
* hardware controls settings, therefore we have to send the corresponding MIDI
* messages to the organ emulator.
* Buttons LEDs shall be switched off.
* The LESLIE 3-position switch and expression pedal positions are read and sent.
*/
void set_controls_initial_state(void);

/*
* Toggles one or all LEDs of the control panel. All LEDs are toggled if the
* led_idx parameter is not provided.
*
* @param int nbToggles - number of times we want the LEDs to toggle
* @param int led_idx - index of LED to be toggled (example: OVERDRIVE_LED)
*/
void toggle_leds(int nb_toggles, int led_idx = -1);

/*
* Sets Overdrive On/Off.
*
* @param bool on - Overdrive is set ON if true; OFF otherwise
*/
void set_overdrive(bool on);

/*
* B3 control panel switch toggle detection with button LED control.
* 
* @param b3_switch - index of a control panel switch (example: OVERDRIVE_SWITCH)
* @param set_b3_control - pointer to the function which performs the changes (control LED toggle and MIDI message sending)
*/
void on_control_change(int b3_switch, void (*set_b3_control)(bool));

/*
* Overdrive button toggle detection with button LED control.
* On the guenine Hammond B3, this button function is Volume Normal/Soft.
* setBfreeUI implements Overdrive On/Off instead.
*/
void on_overdrive_change(void);

/*
* Leslie control change detection.
*/
void on_leslie_change(void);

/*
* Expression pedal change detection.
* Modifies the volume of upper, lower and pedals.
*/
void on_expression_pedal_change(void);

/*
* Discards spurious MIDI messages sent to setBfree by filtering out
* non significant analog measure variations.
*
* @return true if the pedal position was moved enough to produce a valid change
*/
bool is_expression_pedal_change_significant(void);

/*
* Given an expression pedal analog value, returns a pedal position value.
*
* @return byte - pedal position value
*/
byte get_expr_pedal_position(int anlgMeasure);

/*
* Given a Leslie position analog value, returns a STOP/SLOW/FAST value.
*
* @return byte - Leslie switch position value
*/
byte get_leslie_position(int anlgMeasure);

/*
* Sends a MIDI Program Change message over the USB link.
* The MIDI channel is always 0.
*
* @param byte program - MIDI program to be set. setBfree programs can set several parameters.
*/
void send_program_change(byte program);

/*
* Sends a MIDI Control Change message over the USB link.
*
* @param byte channel - MIDI channel the message is sent over
* @param byte control - MIDI controller to be changed
* @param byte value   - new value to be sent to the controlelr
*/
void send_control_change(byte channel, byte control, byte value);

#endif // B3_CONTROLS_H
