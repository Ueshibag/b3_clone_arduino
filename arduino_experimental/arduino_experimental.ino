// MIDI messages for On/Off
#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define UPKB_CHNL 0
#define LWKB_CHNL 1
#define PDKB_CHNL 2

// MIDI keyboards keys velocity
#define VELOCITY_MIN 0
#define VELOCITY_MAX 0x7F

#define C1 36
#define C3 60

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  programChange(UPKB_CHNL, 1);

  // play all pedals
  for (int note = C1; note <= C3; note++)
  {
    sendNote(PDKB_CHNL, note, true);
    delay(300);
    sendNote(PDKB_CHNL, note, false);
    delay(300);
  }
}

void programChange(byte chnl, byte prog)
{
  byte bytes[2];
  bytes[0] = 0xC0 | chnl;
  bytes[1] = prog;
  Serial.write(bytes, 2);
}

/*
   Plays a MIDI note.

   chnl      : MIDI channel the note is sent to
   pitch     : note value
   on        : note ON if true; OFF otherwise
*/
void sendNote(byte chnl, byte pitch, bool on)
{
  byte bytes[3];
  bytes[0] = on ? (NOTE_ON | chnl) : (NOTE_OFF | chnl);
  bytes[1] = pitch;
  bytes[2] = on ? VELOCITY_MAX : VELOCITY_MIN;
  Serial.write(bytes, 3);
}
