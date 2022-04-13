/******************************************************************
              SetBfree Keyboards Control Firmware
                      Arduino Nano Every.

  This B3 clone houses two Fatar keyboards.
  
  Each key contact is made of two switches: when a key is pressed
  the break switch is closed first, then the make switch closes.
  Measuring the delta timing between both events gives information
  about the key velocity. Although the velocity is not taken into
  account in the organ emulation, reading the state of both switches
  is used to implement debouncing.

  Pedals control is no more necessary since I have bought a pedalboard
  from PedaMidiKit which produces MIDI messages by its own.
 ******************************************************************/

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

// rows are the break and make switches lines (16 per keyboard : BR[7:0] + MK[7:0])
#define MATRIX_NB_ROWS (2 * 16)

// maximum number of supported keys
#define KEYBOARDS_NB_PINS (MATRIX_NB_ROWS * MATRIX_NB_COLS)

// MIDI keyboards keys velocity
#define VELOCITY_MIN 0
#define VELOCITY_MAX 0x7F

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

const int BRA = 1; // D1
const int BRB = 2; // D2
const int MKA = 3; // D3
const int MKB = 4; // D4

const int  MUX_A1 = 5; // D5
const int  MUX_A2 = 6; // D6
const int  MUX_A3 = 7; // D7

// snapshot value of all keyboards switches (KEYBOARDS_NB_PINS)
static unsigned long old_value_g[MATRIX_NB_COLS];

static byte note_on_sent_g[KEYBOARDS_NB_PINS / 8];
static byte note_off_sent_g[KEYBOARDS_NB_PINS / 8];

void setup()
{
  setupKeyboardsCtrlPins();

  keyboardsInit();

  Serial.begin(115200);
}

/*
  Sets Arduino Nano Every board pins mode and initial state.
*/
void setupKeyboardsCtrlPins(void)
{
  pinMode(T0, OUTPUT);
  pinMode(T1, OUTPUT);
  pinMode(T2, OUTPUT);
  pinMode(T3, OUTPUT);
  pinMode(T4, OUTPUT);
  pinMode(T5, OUTPUT);
  pinMode(T6, OUTPUT);
  pinMode(T7, OUTPUT);

  digitalWrite(T0, LOW);
  digitalWrite(T1, LOW);
  digitalWrite(T2, LOW);
  digitalWrite(T3, LOW);
  digitalWrite(T4, LOW);
  digitalWrite(T5, LOW);
  digitalWrite(T6, LOW);
  digitalWrite(T7, LOW);

  pinMode(BRA, INPUT);
  pinMode(MKA, INPUT);
  pinMode(BRB, INPUT);
  pinMode(MKB, INPUT);

  pinMode(MUX_A1, OUTPUT);
  pinMode(MUX_A2, OUTPUT);
  pinMode(MUX_A3, OUTPUT);

  digitalWrite(MUX_A1, LOW);
  digitalWrite(MUX_A2, LOW);
  digitalWrite(MUX_A3, LOW);
}

/*
  Initializes keyboards scan engine.
*/
void keyboardsInit(void)
{
  for (int column = 0; column < MATRIX_NB_COLS; column++)
  {
    // default buttons state is: released
    old_value_g[column] = 0x00000000;
  }

  for (int i = 0; i < KEYBOARDS_NB_PINS / 8; i++)
  {
    note_on_sent_g[i] = 0x00;
    note_off_sent_g[i] = 0x00;
  }
}

void loop()
{
  static unsigned long time = 0;
  static unsigned int active_column = 0;

  if (time == 0)
    time = micros();

  unsigned long curTime = micros();

  // if 100us have elapsed, select a new Fatar keyboard matrix column
  if (curTime > time + 100)
  {
    selectKeyboardColumn(active_column);

    // read all switches of both keyboards at a time
    byte switches[4];
    readAllSwitches(switches);

    lookForChanges(switches, active_column);

    time = curTime;
    if (++active_column >= MATRIX_NB_COLS)
      active_column = 0;
  }
}

/*
  Activates one of the T[7:0] Fatar keyboard columns.

  column : the column to be activated (min = 0 / max = 7)
*/
void selectKeyboardColumn(unsigned int column)
{
  digitalWrite(T0, column == 0 ? HIGH : LOW);
  digitalWrite(T1, column == 1 ? HIGH : LOW);
  digitalWrite(T2, column == 2 ? HIGH : LOW);
  digitalWrite(T3, column == 3 ? HIGH : LOW);
  digitalWrite(T4, column == 4 ? HIGH : LOW);
  digitalWrite(T5, column == 5 ? HIGH : LOW);
  digitalWrite(T6, column == 6 ? HIGH : LOW);
  digitalWrite(T7, column == 7 ? HIGH : LOW);
}

/*
  switches is a 4-byte array organized as follows:

                    upper keyboard                                          lower keyboard
  ---------byte 3--------   ---------byte 2--------       ---------byte 1--------   ---------byte 0--------

  bra7 mka7 ... bra4 mka4   bra3 mka3 ... bra0 mka0       brb7 mkb7 ... brb4 mkb4   brb3 mkb3 ... brb0 mkb0
*/
void readAllSwitches(byte* switches)
{
  switches[0] = 0;
  switches[1] = 0;
  switches[2] = 0;
  switches[3] = 0;

  for (byte mux = 0; mux <= 7; mux++)
  {
    digitalWrite(MUX_A1, (mux & 1) == 0 ? LOW : HIGH);
    digitalWrite(MUX_A2, (mux & 2) == 0 ? LOW : HIGH);
    digitalWrite(MUX_A3, (mux & 4) == 0 ? LOW : HIGH);

    int bra = digitalRead(BRA);
    int mka = digitalRead(MKA);

    int brb = digitalRead(BRB);
    int mkb = digitalRead(MKB);

    if (mux <= 3)
    {
      switches[0] |= (mkb << (mux * 2));
      switches[0] |= (brb << ((mux * 2) + 1));
      switches[2] |= (mka << (mux * 2));
      switches[2] |= (bra << ((mux * 2) + 1));
    }
    else
    {
      switches[1] |= (mkb << ((mux - 4) * 2));
      switches[1] |= (brb << (((mux - 4) * 2) + 1));
      switches[3] |= (mka << ((mux - 4) * 2));
      switches[3] |= (bra << (((mux - 4) * 2) + 1));
    }
  }
}

/*
  Looks for keyboards notes changes.

                    upper keyboard                                          lower keyboard
  ---------byte 3--------   ---------byte 2--------       ---------byte 1--------   ---------byte 0--------

  bra7 mka7 ... bra4 mka4   bra3 mka3 ... bra0 mka0       brb7 mkb7 ... brb4 mkb4   brb3 mkb3 ... brb0 mkb0

  col    :  the selected Fatar keyboards column
*/
void lookForChanges(byte* values, byte col)
{
  // make a 32-bit word from 4 bytes
  unsigned long new_value = values[0] & 0xFF;
  new_value |= ((unsigned long)values[1]) << 8;
  new_value |= ((unsigned long)values[2]) << 16;
  new_value |= ((unsigned long)values[3]) << 24;

  unsigned long changed = new_value ^ old_value_g[col];
  if (changed)
  {
    // store new value
    old_value_g[col] = new_value;

    unsigned long mask = 0x01;
    for (byte row = 0; row < MATRIX_NB_ROWS; row++, mask <<= 1)
    {
      if (changed & mask)
        notifyToggle(row, col, (new_value & mask) ? CLOSED : OPEN);
    }
  }
}

/*
  Called whenever a switch state has changed.

  row    : keyboards break and make switches index (0 to 31)
           lower kb make contacts are at row  0, 2, 4, 6, 8, 10, 12, 14
           lower kb break contacts are at row 1, 3, 5, 7, 9, 11, 13, 15
           upper kb make contacts are at row  16, 18, 20, 22, 24, 26, 28, 30
           upper kb break contacts are at row 17, 19, 21, 23, 25, 27, 29, 31

  col    : column index (Fatar keyboards T bus index, 0 to 7)

  closed : true if switch depressed, false if released
*/
void notifyToggle(byte row, byte col, bool closed)
{
  // determine key number, with col[7:0] and row[31:0]
  // key = 127 to 64 for ukb, 63 to 0 for lkb
  int key = 8 * (row / 2) + col;

  // determine MIDI channel
  byte chnl = (key >= 64 ? UPPER : LOWER);

  // check if key is assigned to a break switch
  byte brk = (row & 1); // odd numbers

  // determine pitch (note number of a 61-note keyboard)
  // 36 is the lowest C key value of a 5-octave keyboard (C1)
  // 96 is C6
  int pitch = (key % 64) + 36;

  // ensure valid pitch range
  if (pitch > 96)
    pitch = 96;
  else if (pitch < 0)
    pitch = 0;

  // determine key mask and pointers for access to combined arrays
  byte key_mask = (1 << (key % 8));

  byte* note_on_sent = (byte*) &note_on_sent_g[key / 8];
  byte* note_off_sent = (byte*) &note_off_sent_g[key / 8];

  // break contacts do not send MIDI notes, they release the Note On/Off debouncing mechanism
  if (brk)
  {
    if (closed)
    {
      *note_on_sent &= ~key_mask;
      *note_off_sent &= ~key_mask;
    }
    return;
  }

  // make switch depressed or released?
  if (closed)
  {
    if (!(*note_on_sent & key_mask))
    {
      *note_on_sent |= key_mask;
      sendNote(chnl, pitch, ON);
    }
  }
  else
  {
    if (!(*note_off_sent & key_mask))
    {
      *note_off_sent |= key_mask;
      sendNote(chnl, pitch, OFF);
    }
  }

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
