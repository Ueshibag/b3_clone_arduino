/*
 This is the firmware running on the Arduino Nano Every connected to the
 "Vibrato Chorus & Leslie Controls" board.
 
 The commands sent by this Arduino firmware can be checked by running setBfreeUI.
 Tests are launched by pressing the Volume push button at least 3 seconds.

 Detection of state/level changes on (the ON/OFF state of switches marked
 with * is signaled by a LED.):

 Vibrato-Chorus switches group :
 - volume *
 - vibrato swell *
 - vibrato great *
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
#define CTRL_INIT 127

#define ON 1
#define OFF 0

#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define VELOCITY_MIN 0
#define VELOCITY_MAX 127

#define LESLIE_COMMON_IDX 38

// ------------- MIDI Program values found by running 'setbfree -d' -----------

#define PERCUSSION_OFF 32
#define PERCUSSION_ON 33
#define PERCUSSION_VOLUME_SOFT 34
#define PERCUSSION_VOLUME_NORMAL 35
#define PERCUSSION_DELAY_FAST 36
#define PERCUSSION_DELAY_SLOW 37
#define PERCUSSION_HARMONIC_2 38
#define PERCUSSION_HARMONIC_3 39

#define OVERDRIVE_OFF 40
#define OVERDRIVE_ON 41

#define VIBRATO_V1 42
#define VIBRATO_V2 43
#define VIBRATO_V3 44

#define VIBRATO_C1 45
#define VIBRATO_C2 46
#define VIBRATO_C3 47

#define VIBRATO_GREAT_ON 48
#define VIBRATO_GREAT_OFF 49
#define VIBRATO_SWELL_ON 50
#define VIBRATO_SWELL_OFF 51

#define LESLIE_STOP 52
#define LESLIE_SLOW 53
#define LESLIE_FAST 54

// --------------------------- pins assignments -------------------------------

const int VOLUME_SWITCH = 0;         // D0
const int OVERDRIVE_LED = 1;       // D1
const int VIBRATO_SWELL_SWITCH = 2;  // D2
const int VIBRATO_SWELL_LED = 3;  // D3
const int VIBRATO_GREAT_SWITCH = 4;  // D4
const int VIBRATO_GREAT_LED = 5;  // D5

const int V1 = 6;   // D6
const int C1 = 7;   // D7
const int V2 = 8;   // D8
const int C2 = 9;   // D9
const int V3 = 10;  // D10
const int C3 = 11;  // D11

const int PERC_LED = 12;                // D12
const int PERC_SWITCH = 13;             // D13
const int PERC_VOLUME_LED = 21;  // D21
const int PERC_VOLUME_SWITCH = 20;      // D20
const int PERC_DELAY_LED = 19;     // D19
const int PERC_DELAY_SWITCH = 18;       // D18
const int PERC_HARM_SEL_SWITCH = 17;    // D17
const int PERC_HARM_LED = 16;     // D16
const int EXPR_PEDAL = A1;
const int LESLIE = A0;

// -------------- old_xxx compared to new_xxx to detect changes ---------------

bool old_volume = LOW;
bool new_volume = LOW;
bool led_volume = LOW;

bool old_vibrato_swell = LOW;
bool new_vibrato_swell = LOW;
bool led_vibrato_swell = LOW;

bool old_vibrato_great = LOW;
bool new_vibrato_great = LOW;
bool led_vibrato_great = LOW;

bool old_v1 = LOW;
bool new_v1 = LOW;
bool old_c1 = LOW;
bool new_c1 = LOW;
bool old_v2 = LOW;
bool new_v2 = LOW;
bool old_c2 = LOW;
bool new_c2 = LOW;
bool old_v3 = LOW;
bool new_v3 = LOW;
bool old_c3 = LOW;
bool new_c3 = LOW;

bool old_perc_on = LOW;
bool new_perc_on = LOW;
bool led_perc_on = LOW;

bool old_perc_volume = LOW;
bool new_perc_volume = LOW;
bool led_perc_volume = LOW;

bool old_perc_delay = LOW;
bool new_perc_delay = LOW;
bool led_perc_delay = LOW;

bool old_perc_harm = LOW;
bool new_perc_harm = LOW;
bool led_perc_harm = LOW;

// we send a Leslie MIDI Control Change message on Leslie control change
byte leslie_old = CTRL_INIT;
byte leslie_new = CTRL_INIT;

// we send a control pedal MIDI Control Change message on control pedal move
byte expr_pedal_old;
byte expr_pedal_new;

// we enter test mode by pressing the VOLUME push button at least 3 seconds
unsigned long press_time = 0;
unsigned long release_time = 0;


/*
  Called when a sketch starts. Initializes variables, pin modes, start using libraries, etc.
  The setup() function will only run once, after each powerup or reset of the Arduino board.
*/
void setup() {

    Serial.begin(115200);

    // switches
    pinMode(VOLUME_SWITCH, INPUT);
    pinMode(VIBRATO_SWELL_SWITCH, INPUT);
    pinMode(VIBRATO_GREAT_SWITCH, INPUT);
    pinMode(V1, INPUT_PULLUP);
    pinMode(C1, INPUT_PULLUP);
    pinMode(V2, INPUT_PULLUP);
    pinMode(C2, INPUT_PULLUP);
    pinMode(V3, INPUT_PULLUP);
    pinMode(C3, INPUT_PULLUP);
    pinMode(PERC_SWITCH, INPUT);
    pinMode(PERC_VOLUME_SWITCH, INPUT);
    pinMode(PERC_DELAY_SWITCH, INPUT);
    pinMode(PERC_HARM_SEL_SWITCH, INPUT);

    // analog inputs
    analogReference(EXTERNAL);  // Vdd of the ATmega4809
    pinMode(LESLIE, INPUT);
    pinMode(EXPR_PEDAL, INPUT);

    // the very first call to analogRead() after power up returns junk;
    // this is a documented issue with the ATmega chips.
    analogRead(LESLIE);

    // LEDs
    pinMode(OVERDRIVE_LED, OUTPUT);
    pinMode(VIBRATO_SWELL_LED, OUTPUT);
    pinMode(VIBRATO_GREAT_LED, OUTPUT);
    pinMode(PERC_LED, OUTPUT);
    pinMode(PERC_VOLUME_LED, OUTPUT);
    pinMode(PERC_DELAY_LED, OUTPUT);
    pinMode(PERC_HARM_LED, OUTPUT);
}

/*
  Volume/Overdrive button toggle detection with button LED control.
  We enter test mode by pressing this button more than 3 seconds.
*/
void onVolumeChange() {

    new_volume = digitalRead(VOLUME_SWITCH);

    if (new_volume != old_volume) {

        if (new_volume == LOW) {

            led_volume = !led_volume;
            digitalWrite(OVERDRIVE_LED, led_volume);

            if (led_volume)
                sendProgramChange(0, OVERDRIVE_ON);
            else
                sendProgramChange(0, OVERDRIVE_OFF);

            release_time = millis();
        }

        else {
            press_time = millis();
        }

        old_volume = new_volume;
    }

    long on_time = release_time - press_time;

    if (on_time > 3000)
        enterTestMode();
}

/*
  Vibrato swell (upper manual) toggle detection with button LED control.
*/
void onVibratoSwellChange() {

    new_vibrato_swell = digitalRead(VIBRATO_SWELL_SWITCH);

    if (new_vibrato_swell != old_vibrato_swell) {

        if (new_vibrato_swell == LOW) {

            led_vibrato_swell = !led_vibrato_swell;
            digitalWrite(VIBRATO_SWELL_LED, led_vibrato_swell);

            if (led_vibrato_swell)
                sendProgramChange(0, VIBRATO_SWELL_ON);

            else
                sendProgramChange(0, VIBRATO_SWELL_OFF);
        }
        old_vibrato_swell = new_vibrato_swell;
    }
}

/*
  Vibrato great (lower manual) toggle detection with button LED control.
*/
void onVibratoGreatChange() {

    new_vibrato_great = digitalRead(VIBRATO_GREAT_SWITCH);

    if (new_vibrato_great != old_vibrato_great) {

        if (new_vibrato_great == LOW) {
            led_vibrato_great = !led_vibrato_great;
            digitalWrite(VIBRATO_GREAT_LED, led_vibrato_great);

            if (led_vibrato_great)
                sendProgramChange(0, VIBRATO_GREAT_ON);

            else
                sendProgramChange(0, VIBRATO_GREAT_OFF);
        }
        old_vibrato_great = new_vibrato_great;
    }
}

/*
  The vibrato/chorus rotary switch is set on V1.
*/
void onV1Change() {

    new_v1 = digitalRead(V1);

    if (new_v1 != old_v1) {

        if (new_v1 == LOW)
            sendProgramChange(0, VIBRATO_V1);

        old_v1 = new_v1;
    }
}

/*
  The vibrato/chorus rotary switch is set on C1.
*/
void onC1Change() {

    new_c1 = digitalRead(C1);

    if (new_c1 != old_c1) {

        if (new_c1 == LOW)
            sendProgramChange(0, VIBRATO_C1);

        old_c1 = new_c1;
    }
}

/*
  The vibrato/chorus rotary switch is set on V2.
*/
void onV2Change() {

    new_v2 = digitalRead(V2);

    if (new_v2 != old_v2) {

        if (new_v2 == LOW)
            sendProgramChange(0, VIBRATO_V2);

        old_v2 = new_v2;
    }
}

/*
  The vibrato/chorus rotary switch is set on C2.
*/
void onC2Change() {

    new_c2 = digitalRead(C2);

    if (new_c2 != old_c2) {

        if (new_c2 == LOW)
            sendProgramChange(0, VIBRATO_C2);

        old_c2 = new_c2;
    }
}

/*
  The vibrato/chorus rotary switch is set on V3.
*/
void onV3Change() {

    new_v3 = digitalRead(V3);

    if (new_v3 != old_v3) {

        if (new_v3 == LOW)
            sendProgramChange(0, VIBRATO_V3);

        old_v3 = new_v3;
    }
}

/*
  The vibrato/chorus rotary switch is set on V1.
*/
void onC3Change() {

    new_c3 = digitalRead(C3);

    if (new_c3 != old_c3) {

        if (new_c3 == LOW)
            sendProgramChange(0, VIBRATO_C3);

        old_c3 = new_c3;
    }
}

/*
  Percussion ON/OFF button toggle detection with button LED control.
*/
void onPercussionChange() {

    new_perc_on = digitalRead(PERC_SWITCH);

    if (new_perc_on != old_perc_on) {

        if (new_perc_on == LOW) {

            led_perc_on = !led_perc_on;
            digitalWrite(PERC_LED, led_perc_on);

            if (led_perc_on)
                sendProgramChange(0, PERCUSSION_ON);
            else
                sendProgramChange(0, PERCUSSION_OFF);
        }
        old_perc_on = new_perc_on;
    }
}

/*
  Percussion volume SOFT/NORMAL button toggle detection with button LED control.
*/
void onPercussionVolumeChange() {

    new_perc_volume = digitalRead(PERC_VOLUME_SWITCH);

    if (new_perc_volume != old_perc_volume) {

        if (new_perc_volume == LOW) {

            led_perc_volume = !led_perc_volume;
            digitalWrite(PERC_VOLUME_LED, led_perc_volume);

            if (led_perc_volume)
                sendProgramChange(0, PERCUSSION_VOLUME_SOFT);
            else
                sendProgramChange(0, PERCUSSION_VOLUME_NORMAL);
        }
        old_perc_volume = new_perc_volume;
    }
}

/*
  Percussion delay FAST/SLOW button toggle detection with button LED control.
*/
void onPercussionDelayChange() {

    new_perc_delay = digitalRead(PERC_DELAY_SWITCH);

    if (new_perc_delay != old_perc_delay) {

        if (new_perc_delay == LOW) {

            led_perc_delay = !led_perc_delay;
            digitalWrite(PERC_DELAY_LED, led_perc_delay);

            if (led_perc_delay)
                sendProgramChange(0, PERCUSSION_DELAY_FAST);
            else
                sendProgramChange(0, PERCUSSION_DELAY_SLOW);
        }
        old_perc_delay = new_perc_delay;
    }
}

/*
  Percussion harmonic THIRD/SECOND button toggle detection with button LED control.
*/
void onPercussionHarmonicChange() {

    new_perc_harm = digitalRead(PERC_HARM_SEL_SWITCH);

    if (new_perc_harm != old_perc_harm) {

        if (new_perc_harm == LOW) {

            led_perc_harm = !led_perc_harm;
            digitalWrite(PERC_HARM_LED, led_perc_harm);

            if (led_perc_harm)
                sendProgramChange(0, PERCUSSION_HARMONIC_3);
            else
                sendProgramChange(0, PERCUSSION_HARMONIC_2);
        }
        old_perc_harm = new_perc_harm;
    }
}

/*
  Leslie control change detection.
*/
void onLeslieChange() {

    int anlgMeasure = analogRead(LESLIE);
    leslie_new = getLesliePosition(anlgMeasure);

    if (leslie_new != leslie_old) {

        sendProgramChange(0, leslie_new);
        leslie_old = leslie_new;
    }
}

/*
  Given a Leslie position analog value, returns a STOP/SLOW/FAST value.
*/
byte getLesliePosition(int anlgMeasure) {

    byte position = LESLIE_STOP;

    if (anlgMeasure >= 0 && anlgMeasure < 50)
        position = LESLIE_SLOW;

    else if (anlgMeasure > 990 && anlgMeasure < 1030)
        position = LESLIE_FAST;

    return position;
}

/*
  Expression pedal change detection. Modifies the volume of upper manual only.
*/
void onExpressionPedalChange() {

    int anlgMeasure = analogRead(EXPR_PEDAL);
    expr_pedal_new = getExprPedalPosition(anlgMeasure);

    if (expr_pedal_new != expr_pedal_old) {

        sendControlChange(0, 7, expr_pedal_new);
        expr_pedal_old = expr_pedal_new;
    }
}

/*
  Given an expression pedal analog value, returns a pedal position value.
*/
byte getExprPedalPosition(int anlgMeasure) {

    byte position = anlgMeasure / 256;
    return position;
}

/*
  Sends a MIDI Control Change message over the USB link.
*/
void sendControlChange(byte channel, byte control, byte value) {

    byte bytes[3];
    bytes[0] = 0xB0 | channel;
    bytes[1] = control;
    bytes[2] = value;
    Serial.write(bytes, 3);
}

/*
  Sends a MIDI Program Change message over the USB link.
*/
void sendProgramChange(byte channel, byte program) {

    byte bytes[2];
    bytes[0] = 0xC0 | channel;
    bytes[1] = program;
    Serial.write(bytes, 2);
}

/*
  Commands effects can be checked on the setBfree UI version.
  One should see setBfree UI buttons toggling.
  The test sequence starts and ends up with all LEDs blinking 3 times.
  READ IMPORTANT INFORMATION ABOUT setBfree default.cfg ON TOP OF THIS FILE.
*/
void enterTestMode() {

    toggleLeds(3);
    release_time = 0;
    press_time = 0;

    sendProgramChange(0, OVERDRIVE_ON);
    digitalWrite(OVERDRIVE_LED, ON);
    delay(2000);

    sendProgramChange(0, OVERDRIVE_OFF);
    digitalWrite(OVERDRIVE_LED, OFF);
    delay(2000);

    sendProgramChange(0, VIBRATO_SWELL_ON);
    digitalWrite(VIBRATO_SWELL_LED, ON);
    delay(2000);

    sendProgramChange(0, VIBRATO_SWELL_OFF);
    digitalWrite(VIBRATO_SWELL_LED, OFF);
    delay(2000);

    sendProgramChange(0, VIBRATO_GREAT_ON);
    digitalWrite(VIBRATO_GREAT_LED, ON);
    delay(2000);

    sendProgramChange(0, VIBRATO_GREAT_OFF);
    digitalWrite(VIBRATO_GREAT_LED, OFF);
    delay(2000);

    sendProgramChange(0, VIBRATO_V1);
    delay(2000);

    sendProgramChange(0, VIBRATO_V2);
    delay(2000);

    sendProgramChange(0, VIBRATO_V3);
    delay(2000);

    sendProgramChange(0, VIBRATO_C1);
    delay(2000);

    sendProgramChange(0, VIBRATO_C2);
    delay(2000);

    sendProgramChange(0, VIBRATO_C3);
    delay(2000);

    sendProgramChange(0, PERCUSSION_ON);
    digitalWrite(PERC_LED, ON);
    delay(2000);

    sendProgramChange(0, PERCUSSION_OFF);
    digitalWrite(PERC_LED, OFF);
    delay(2000);

    sendProgramChange(0, PERCUSSION_VOLUME_SOFT);
    digitalWrite(PERC_VOLUME_LED, ON);
    delay(2000);

    sendProgramChange(0, PERCUSSION_VOLUME_NORMAL);
    digitalWrite(PERC_VOLUME_LED, OFF);
    delay(2000);

    sendProgramChange(0, PERCUSSION_DELAY_FAST);
    digitalWrite(PERC_DELAY_LED, ON);
    delay(2000);

    sendProgramChange(0, PERCUSSION_DELAY_SLOW);
    digitalWrite(PERC_DELAY_LED, OFF);
    delay(2000);
  
    sendProgramChange(0, PERCUSSION_HARMONIC_3);
    digitalWrite(PERC_HARM_LED, ON);
    delay(2000);

    sendProgramChange(0, PERCUSSION_HARMONIC_2);
    digitalWrite(PERC_HARM_LED, OFF);
    delay(2000);

    sendProgramChange(0, LESLIE_SLOW);
    delay(2000);

    sendProgramChange(0, LESLIE_FAST);
    delay(2000);

    sendProgramChange(0, LESLIE_STOP);
    delay(2000);

    toggleLeds(3);
}

/*
  Signals we enter or exit test mode by toggling the control panel switches
  LEDs on and off.

  nb_toggles:   number of times we toggle LEDs.
*/
void toggleLeds(int nb_toggles) {

    for (int i = 0; i < nb_toggles; i++) {

        digitalWrite(OVERDRIVE_LED, ON);
        digitalWrite(VIBRATO_SWELL_LED, ON);
        digitalWrite(VIBRATO_GREAT_LED, ON);

        digitalWrite(PERC_LED, ON);
        digitalWrite(PERC_VOLUME_LED, ON);
        digitalWrite(PERC_DELAY_LED, ON);
        digitalWrite(PERC_HARM_LED, ON);

        delay(200);

        digitalWrite(OVERDRIVE_LED, OFF);
        digitalWrite(VIBRATO_SWELL_LED, OFF);
        digitalWrite(VIBRATO_GREAT_LED, OFF);

        digitalWrite(PERC_LED, OFF);
        digitalWrite(PERC_VOLUME_LED, OFF);
        digitalWrite(PERC_DELAY_LED, OFF);
        digitalWrite(PERC_HARM_LED, OFF);

        delay(200);
    }
}

/*
  The loop() function does precisely what its name suggests, and loops consecutively,
  allowing your program to change and respond.
*/
void loop() {

    static unsigned long time = 0;

    if (time == 0)
        time = micros();

    unsigned long curTime = micros();

    if (curTime > time + 100) {

        onVolumeChange();
        onVibratoSwellChange();
        onVibratoGreatChange();
        onV1Change();
        onC1Change();
        onV2Change();
        onC2Change();
        onV3Change();
        onC3Change();
        onPercussionChange();
        onPercussionVolumeChange();
        onPercussionDelayChange();
        onPercussionHarmonicChange();
        onLeslieChange();

        // keep the following line commented out while the expression pedal remains unconnected
        // to prevent spurious invalid readings
        // onExpressionPedalChange();

        time = curTime;
    }
}
