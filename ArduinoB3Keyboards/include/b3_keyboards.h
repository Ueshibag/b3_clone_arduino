// ===========================================================================
// b3_keyboards.h
// includes other project header files
// ===========================================================================
#ifndef B3_KEYBOARDS_H
#define B3_KEYBOARDS_H

#include <Arduino.h>

// MIDI channels
#define UPPER 0
#define LOWER 1

// Note On/Off
#define ON true
#define OFF false

#define CLOSED true
#define OPEN false

// MIDI messages for On/Off
#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define BRA 1  // D1
#define BRB 2  // D2
#define MKA 3  // D3
#define MKB 4  // D4

#define MUX_A1 5  // D5
#define MUX_A2 6  // D6
#define MUX_A3 7  // D7

// columns are the Fatar keyboards T[7:0] lines
#define MATRIX_NB_COLS 8
#define T0 A0
#define T1 A1
#define T2 A2
#define T3 A3
#define T4 A4
#define T5 A5
#define T6 A6
#define T7 A7

// rows are the break and make switches lines (16 per keyboard : BR[7:0] + MK[7:0])
#define MATRIX_NB_ROWS (2 * 16) // 2 keyboards !!

// maximum number of supported keys
#define KEYBOARDS_NB_PINS (MATRIX_NB_ROWS * MATRIX_NB_COLS)

// MIDI keyboards keys velocity
#define VELOCITY_MIN 0
#define VELOCITY_MAX 0x7F


/*
  Sets Arduino Nano Every board pins mode and initial state.
*/
void setup_keyboards_ctrl_pins(void);


/*
  Initializes keyboards scan engine.
*/
void init_keyboards(void);

/*
  Activates one of the T[7:0] Fatar keyboard columns.

  column : the column to be activated (min = 0 / max = 7)
*/
void select_keyboard_column(unsigned int column);


/*
  switches is a 4-byte array organized as follows:

                    upper keyboard                                          lower keyboard
  ---------byte 3--------   ---------byte 2--------       ---------byte 1--------   ---------byte 0--------

  bra7 mka7 ... bra4 mka4   bra3 mka3 ... bra0 mka0       brb7 mkb7 ... brb4 mkb4   brb3 mkb3 ... brb0 mkb0
*/
void read_all_switches(byte* switches);


/*
  Looks for keyboards notes changes.

                    upper keyboard                                          lower keyboard
  ---------byte 3--------   ---------byte 2--------       ---------byte 1--------   ---------byte 0--------

  bra7 mka7 ... bra4 mka4   bra3 mka3 ... bra0 mka0       brb7 mkb7 ... brb4 mkb4   brb3 mkb3 ... brb0 mkb0

  col    :  the selected Fatar keyboards column
*/
void look_for_changes(byte* values, byte col);


/*
  Called whenever a switch state has changed.

  @param row    : keyboards break and make switches index (0 to 31)
                  lower kb make contacts are at row  0, 2, 4, 6, 8, 10, 12, 14
                  lower kb break contacts are at row 1, 3, 5, 7, 9, 11, 13, 15
                  upper kb make contacts are at row  16, 18, 20, 22, 24, 26, 28, 30
                  upper kb break contacts are at row 17, 19, 21, 23, 25, 27, 29, 31

  @param col    : column index (Fatar keyboards T bus index, 0 to 7)

  @param closed : true if switch depressed, false if released
*/
void notify_toggle(byte row, byte col, bool closed);

/*
   Plays a MIDI note.

   @param chnl    : MIDI channel the note is sent to
   @param pitch   : note value
   @param on      : note ON if true; OFF otherwise
*/
void send_note(byte chnl, byte pitch, bool on);


#endif // B3_KEYBOARDS_H
