/*
 This is the firmware running on the Arduino Nano Every connected to the "Vibrato Chorus & Leslie Controls" board.
 
 Detection of state/level changes on :

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

 The ON/OFF state of switches marked with * is signaled by a LED.
*/
#define CTRL_INIT 127

#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define VELOCITY_MIN 0
#define VELOCITY_MAX 0x7F

// Leslie constants
#define LESLIE_COMMON_IDX 38
#define LESLIE_STOP 52
#define LESLIE_SLOW 53
#define LESLIE_FAST 54

#define OVERDRIVE_OFF 40
#define OVERDRIVE_ON  41

#define VIBRATO_GREAT_ON  48
#define VIBRATO_GREAT_OFF 49
#define VIBRATO_SWELL_ON  50
#define VIBRATO_SWELL_OFF 51

const int VOLUME_SWITCH = 0;            // D0
const int VOLUME_SOFT_LED = 1;          // D1
const int VIBRATO_SWELL_SWITCH = 2;     // D2
const int VIBRATO_SWELL_ON_LED = 3;     // D3
const int VIBRATO_GREAT_SWITCH = 4;     // D4
const int VIBRATO_GREAT_ON_LED = 5;     // D5

const int V1 = 6;                       // D6
const int C1 = 7;                       // D7
const int V2 = 8;                       // D8
const int C2 = 9;                       // D9
const int V3 = 10;                      // D10
const int C3 = 11;                      // D11

const int PERC_ON_LED = 12;             // D12
const int PERC_SWITCH = 13;             // D13
const int PERC_VOLUME_NORMAL_LED = 21;  // D21
const int PERC_VOLUME_SWITCH = 20;      // D20
const int PERC_DELAY_FAST_LED = 19;     // D19
const int PERC_DELAY_SWITCH = 18;       // D18
const int PERC_HARM_SEL_SWITCH = 17;    // D17
const int PERC_HARM_THIRD_LED = 16;     // D16
const int EXPR_PEDAL = A1;
const int LESLIE = A0;

// old_xxx compared to new_xxx to detect changes
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
byte leslie_old;
byte leslie_new;

byte expr_pedal_old;
byte expr_pedal_new;

void sendControlChange(byte channel, byte control, byte value)
{
    byte bytes[3];
    bytes[0] = 0xB0 | channel;
    bytes[1] = control;
    bytes[2] = value;
    Serial.write(bytes, 3);
}

void sendProgramChange(byte channel, byte program)
{
    byte bytes[2];
    bytes[0] = 0xC0 | channel;
    bytes[1] = program;
    Serial.write(bytes, 2);
}

void sendNoteOn(byte channel, byte pitch)
{
    byte bytes[3];
    bytes[0] = NOTE_ON | channel;
    bytes[1] = pitch;
    bytes[2] = VELOCITY_MAX;
    Serial.write(bytes, 3);
}

void sendNoteOff(byte channel, byte pitch)
{
    byte bytes[3];
    bytes[0] = NOTE_OFF | channel;
    bytes[1] = pitch;
    bytes[2] = VELOCITY_MIN;
    Serial.write(bytes, 3);
}

void setup()
{
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
  analogReference(EXTERNAL); // Vdd of the ATmega4809
  pinMode(LESLIE, INPUT);
  pinMode(EXPR_PEDAL, INPUT);
  
  // the very first call to analogRead() after power up returns junk;
  // this is a documented issue with the ATmega chips.
  analogRead(LESLIE);

  // LEDs
  pinMode(VOLUME_SOFT_LED, OUTPUT);
  pinMode(VIBRATO_SWELL_ON_LED, OUTPUT);
  pinMode(VIBRATO_GREAT_ON_LED, OUTPUT);
  pinMode(PERC_ON_LED, OUTPUT);
  pinMode(PERC_VOLUME_NORMAL_LED, OUTPUT);
  pinMode(PERC_DELAY_FAST_LED, OUTPUT);
  pinMode(PERC_HARM_THIRD_LED, OUTPUT);  
  
  // turn all LEDs off
  digitalWrite(VOLUME_SOFT_LED, LOW);
  digitalWrite(VIBRATO_SWELL_ON_LED, LOW);
  digitalWrite(VIBRATO_GREAT_ON_LED, LOW);
  digitalWrite(PERC_ON_LED, LOW);
  digitalWrite(PERC_VOLUME_NORMAL_LED, LOW);
  digitalWrite(PERC_DELAY_FAST_LED, LOW);
  digitalWrite(PERC_HARM_THIRD_LED, LOW);

  leslie_old = CTRL_INIT;
  leslie_new = CTRL_INIT;
}

void onVolumeChange()
{
  new_volume = digitalRead(VOLUME_SWITCH);
  if (new_volume != old_volume)
  {
    if (new_volume == LOW)
    {  
      led_volume = ! led_volume;
      digitalWrite(VOLUME_SOFT_LED, led_volume);

      if (led_volume)      
        sendProgramChange(0, OVERDRIVE_ON);
      else
        sendProgramChange(0, OVERDRIVE_OFF);      
    }
    old_volume = new_volume;        
  }
}

void onVibratoSwellChange()
{
  new_vibrato_swell = digitalRead(VIBRATO_SWELL_SWITCH);
  if (new_vibrato_swell != old_vibrato_swell)
  {
    if (new_vibrato_swell == LOW)
    {   
      led_vibrato_swell = ! led_vibrato_swell;
      digitalWrite(VIBRATO_SWELL_ON_LED, led_vibrato_swell);

      if (led_vibrato_swell)      
        sendProgramChange(0, VIBRATO_SWELL_ON);
      else
        sendProgramChange(0, VIBRATO_SWELL_OFF);    
    }
    old_vibrato_swell = new_vibrato_swell;
  }
}

void onVibratoGreatChange()
{
  new_vibrato_great = digitalRead(VIBRATO_GREAT_SWITCH);
  if (new_vibrato_great != old_vibrato_great)
  {
    if (new_vibrato_great == LOW)
    {   
      led_vibrato_great = ! led_vibrato_great;
      digitalWrite(VIBRATO_GREAT_ON_LED, led_vibrato_great);

      if (led_vibrato_great)
        sendProgramChange(0, VIBRATO_GREAT_ON);
      else
        sendProgramChange(0, VIBRATO_GREAT_OFF);
    }
    old_vibrato_great = new_vibrato_great;
  }
}

void onV1Change()
{
  new_v1 = digitalRead(V1);
  if (new_v1 != old_v1)
  {
    if (new_v1 == LOW)
    {
      // switch closed      

    }
    old_v1 = new_v1;
  }
}

void onC1Change()
{
  new_c1 = digitalRead(C1);
  if (new_c1 != old_c1)
  {
    if (new_c1 == LOW)
    {
      // switch closed      

    }
    old_c1 = new_c1;
  }
}

void onV2Change()
{
  new_v2 = digitalRead(V2);
  if (new_v2 != old_v2)
  {
    if (new_v2 == LOW)
    {
      // switch closed      

    }
    old_v2 = new_v2;
  }
}

void onC2Change()
{
  new_c2 = digitalRead(C2);
  if (new_c2 != old_c2)
  {
    if (new_c2 == LOW)
    {
      // switch closed      

    }
    old_c2 = new_c2;
  }
}

void onV3Change()
{
  new_v3 = digitalRead(V3);
  if (new_v3 != old_v3)
  {
    if (new_v3 == LOW)
    {
      // switch closed      

    }
    old_v3 = new_v3;
  }
}

void onC3Change()
{
  new_c3 = digitalRead(C3);
  if (new_c3 != old_c3)
  {
    if (new_c3 == LOW)
    {
      // switch closed      

    }
    old_c3 = new_c3;
  }    
}

void onPercussionChange()
{
  new_perc_on = digitalRead(PERC_SWITCH);
  if (new_perc_on != old_perc_on)
  {
    if (new_perc_on == LOW)
    {
      // switch closed      
      led_perc_on = ! led_perc_on;
      digitalWrite(PERC_ON_LED, led_perc_on);
    }
    old_perc_on = new_perc_on;        
  }
}

void onPercussionVolumeChange()
{
  new_perc_volume = digitalRead(PERC_VOLUME_SWITCH);
  if (new_perc_volume != old_perc_volume)
  {
    if (new_perc_volume == LOW)
    {
      // switch closed      
      led_perc_volume = ! led_perc_volume;
      digitalWrite(PERC_VOLUME_NORMAL_LED, led_perc_volume);
    }
    old_perc_volume = new_perc_volume;        
  }
}

void onPercussionDelayChange()
{
  new_perc_delay = digitalRead(PERC_DELAY_SWITCH);
  if (new_perc_delay != old_perc_delay)
  {
    if (new_perc_delay == LOW)
    {
      // switch closed      
      led_perc_delay = ! led_perc_delay;
      digitalWrite(PERC_DELAY_FAST_LED, led_perc_delay);
    }
    old_perc_delay = new_perc_delay;        
  }
}

void onPercussionHarmonicChange()
{
  new_perc_harm = digitalRead(PERC_HARM_SEL_SWITCH);
  if (new_perc_harm != old_perc_harm)
  {
    if (new_perc_harm == LOW)
    {
      // switch closed      
      led_perc_harm = ! led_perc_harm;
      digitalWrite(PERC_HARM_THIRD_LED, led_perc_harm);
    }
    old_perc_harm = new_perc_harm;        
  }
}

byte getLesliePosition(int anlgMeasure)
{
  byte position = LESLIE_STOP;
  
  if (anlgMeasure >= 0 && anlgMeasure < 50)
    position = LESLIE_SLOW;

  else if (anlgMeasure > 990 && anlgMeasure < 1030)
    position =  LESLIE_FAST;

  return position;
}

byte getExprPedalPosition(int anlgMeasure)
{
  byte position = anlgMeasure / 256;

  return position;
}

void onLeslieChange()
{
  int anlgMeasure = analogRead(LESLIE);
  leslie_new = getLesliePosition(anlgMeasure);
  
  if (leslie_new != leslie_old)
  {
    sendProgramChange(0, leslie_new);
    leslie_old = leslie_new;
  }
}

void onExpressionPedalChange()
{
  int anlgMeasure = analogRead(EXPR_PEDAL);
  expr_pedal_new = getExprPedalPosition(anlgMeasure);
  
  if (expr_pedal_new != expr_pedal_old)
  {
    sendProgramChange(0, expr_pedal_new);
    expr_pedal_old = expr_pedal_new;
  }
}


void loop()
{
  static unsigned long time = 0;

  if (time == 0)
    time = micros();

  unsigned long curTime = micros();
  
  if (curTime > time + 100)
  {
    onVolumeChange();
    onVibratoSwellChange(); 
    onVibratoGreatChange(); // send note / stop note

    time = curTime;
  }
}
