#ifndef B3_PERCUSSION_H
#define B3_PERCUSSION_H

#define ON 1
#define OFF 0

// percussion volume
#define SOFT 1
#define NORMAL 0

// percussion delay
#define FAST 1
#define SLOW 0

// percussion harmonic
#define THIRD 1
#define SECOND 0

#define PERCUSSION_OFF 32
#define PERCUSSION_ON 33
#define PERCUSSION_VOLUME_SOFT 34
#define PERCUSSION_VOLUME_NORMAL 35
#define PERCUSSION_DELAY_FAST 36
#define PERCUSSION_DELAY_SLOW 37
#define PERCUSSION_HARMONIC_2 38
#define PERCUSSION_HARMONIC_3 39

/*
* Sets Percussion On/Off.
*
* @param bool on - Percussion is set ON if true; OFF otherwise
*/
void set_percussion(bool on);

void set_percussion_volume(bool soft);

void set_percussion_delay(bool fast);

void set_percussion_harmonic(bool third);

/*
* Percussion ON/OFF button toggle detection with button LED control.
*/
void on_percussion_change(void);

/*
* Percussion volume SOFT/NORMAL button toggle detection with button LED control.
*/
void on_percussion_volume_change(void);

/*
* Percussion delay FAST/SLOW button toggle detection with button LED control.
*/
void on_percussion_delay_change(void);

/*
* Percussion harmonic THIRD/SECOND button toggle detection with button LED control.
*/
void on_percussion_harmonic_change(void);

#endif
