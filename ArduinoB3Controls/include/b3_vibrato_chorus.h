#ifndef B3_VIBRATO_CHORUS_H
#define B3_VIBRATO_CHORUS_H

#define V1 45
#define C1 46
#define V2 47
#define C2 42
#define V3 43
#define C3 44

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
*/
void on_vibrato_chorus_change(void);

#endif
