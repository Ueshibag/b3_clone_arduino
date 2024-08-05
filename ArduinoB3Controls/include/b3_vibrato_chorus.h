#ifndef B3_VIBRATO_CHORUS_H
#define B3_VIBRATO_CHORUS_H

#define VIBRATO_V1 42
#define VIBRATO_V2 43
#define VIBRATO_V3 44

#define VIBRATO_C1 45
#define VIBRATO_C2 46
#define VIBRATO_C3 47

#define VIBRATO_LOWER_ON 48
#define VIBRATO_LOWER_OFF 49
#define VIBRATO_UPPER_ON 50
#define VIBRATO_UPPER_OFF 51

void set_vibrato_upper(bool on);

void set_vibrato_lower(bool on);

/*
* Vibrato upper manual toggle detection with button LED control.
*/
void on_vibrato_upper_change(void);

/*
* Vibrato lower manual toggle detection with button LED control.
*/
void on_vibrato_lower_change(void);

/*
* The vibrato/chorus rotary switch is set on V[1-3] or C[1-3].
* @param vc_pin - Arduino digital input pin connected to Vx/Cx
* @param vc_prog - MIDI program to be set
*/
void on_vibrato_chorus_change(int vc_pin, int vc_prog);

#endif
