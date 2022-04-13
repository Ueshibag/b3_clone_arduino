/*************************************************************************
                SetBfree Harmonic Drawbars Control.

  From bass to treble pipes, drawbars names are the following:
  Controller indexes are decimal values

  Controller      Function
      70          drawbar16
      71          drawbar513
      72          drawbar8
      73          drawbar4
      74          drawbar223
      75          drawbar2
      76          drawbar135
      77          drawbar113
      78          drawbar1

  Drawbar position decimal values of the MIDI Control Change:
  8 = 00    (loudest)
  7 = 01-15
  6 = 16-31
  5 = 32-47
  4 = 48-63
  3 = 64-79
  2 = 80-92
  1 = 96-110
  0 = 111-127   (off)

  Examples:
  B0 70 00  = [Control Change Channel 0] [drawbar16] [drawbar position 8]
  B0 70 111 = [Control Change Channel 0] [drawbar16] [drawbar position 0]
  B0 72 80  = [Control Change Channel 0] [drawbar8]  [drawbar position 2]

 *************************************************************************

  drawbarIdx    anlgIdx    mux
  =============================
        0           0       0       UM_1_1
        1           0       1
        2           0       2
        3           0       3
        4           0       4
        5           0       5
        6           0       6
        7           0       7       UM_1_8

        8           1       0       UM_1_9
        9           1       1       UM_2_1
       10           1       2
       11           1       3
       12           1       4
       13           1       5
       14           1       6
       15           1       7       UM_2_7

       16           2       0       UM_2_8
       17           2       1       UM_2_9
       18           2       2       PK_1
       19           2       3       PK_2
       20           2       4       LM_1_1
       21           2       5
       22           2       6
       23           2       7       LM_1_4

       24           3       0       LM_1_5
       25           3       1
       26           3       2
       27           3       3
       28           3       4       LM_1_9
       29           3       5       LM_2_1
       30           3       6
       31           3       7       LM_2_3

       32           4       0       LM_2_4
       33           4       1
       34           4       2
       35           4       3
       36           4       4
       37           4       5       LM_2_9
       38           4       6          X
       39           4       7          X
*/

// MIDI channels
// Drawbars moves messages are not sent directly as MIDI messages to setBfree
// because we have two sets of drawbars per manual keyboard and therefore we
// want to know which set is in use. For that purpose, I assign temporary channels
// to drawbars sets and the RPI JavaFX controller will manage the actual MIDI
// channel assignment to be sent to setBfree.
#define UPPER_1 0
#define UPPER_2 3
#define LOWER_1 1
#define LOWER_2 4
#define PEDAL 2

#define NB_DRAWBARS 38
#define ACK 0x06

// Leslie constants
#define LESLIE_COMMON_IDX 38
#define LESLIE_STOP 52
#define LESLIE_SLOW 53
#define LESLIE_FAST 54

#define CTRL_INIT 127

// multiplexers controls
const int  MUX_A = 2; // D2
const int  MUX_B = 3; // D3
const int  MUX_C = 4; // D4

// analog reading with average calculation
int analogPins[5] = {A1, A2, A3, A4, A5};

// we have 38 drawbars and every one has 9 possible positions, which can be coded on one byte
// we send a MIDI message only when a drawbar position has changed
byte pos_old[NB_DRAWBARS];
byte pos_new[NB_DRAWBARS];

// we send a Leslie MIDI Control Change message only when Leslie settings have changed
byte leslie_old;
byte leslie_new;

//                    0    1   2   3   4   5   6   7  8
byte dbar_pos[9] = {127, 110, 92, 79, 63, 47, 31, 15, 0};

/*
   Arduino program setup. This function is executed only once.
*/
void setup()
{
  setupCtrlPins();
  analogReference(EXTERNAL); // Vdd of the ATmega4809
  Serial.begin(115200);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // wait for Raspberry PI so that setBfree can listen to MIDI messages
  syncRPI();

  // the very first call to analogRead() after powering up returns junk,
  // this is a documented issue with the ATmega chips.
  int anlgMeasure = analogRead(analogPins[0]);
  
  // read and send drawbars and Leslie initial positions
  for (int mux = 0; mux < 8; mux++)
  {
    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);

    sendInitialControlsPositions(mux);
  }
}

/*
   Arduino program main loop.
*/
void loop()
{
  static unsigned long time = 0;
  static byte mux = 0;

  if (time == 0)
    time = micros();

  unsigned long curTime = micros();

  // if 500us have elapsed, select a new mux
  if (curTime > time + 2000)
  {
    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);

    onControlsChange(mux);

    time = curTime;

    if (++mux >= 8)
      mux = 0;    // mux = 0, 1, 2, 3, 4, 5, 6, 7, 0, ...
  }
}

void setupCtrlPins(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);

  digitalWrite(MUX_A, LOW);
  digitalWrite(MUX_B, LOW);
  digitalWrite(MUX_C, LOW);

  for (int i = 0; i < NB_DRAWBARS; i++)
  {
    pos_old[i] = CTRL_INIT;
    pos_new[i] = CTRL_INIT;
  }

  leslie_old = CTRL_INIT;
  leslie_new = CTRL_INIT;
}

/*
   Repeatidly sends a sync byte until RPI replies.
*/
void syncRPI()
{
  int rcv = 0;
  do
  {
    // send Enquiry to RPI until it replies with ACK
    while (Serial.available() <= 0)
    {
      Serial.write('E'); // 0x45
      delay(300);
    }
    rcv = Serial.read();
  }
  while (rcv != ACK);
}

/*
  We are reading drawbars and Leslie initial positions and therefore we send them
  without checking for positions changes.
  Reads the 5 analog inputs of the ATMEGA4809 for each mux value.
  Sends MIDI control change message to the corresponding MIDI controller.

  mux : multiplexer value used to read one analog value among eight
        mux values are [0..7]
*/
void sendInitialControlsPositions(byte mux)
{
  //                       -- UPPER --    BASS    --- LOWER ---
  // drawbars array index [0..8][9..17] [18..19] [20..28][29..37]
  int ctrlIdx = 0;

  // read the 5 analog pins sequentially
  for (byte anlgIdx = 0; anlgIdx <= 4; anlgIdx++)
  {
    ctrlIdx = anlgIdx * 8 + mux;
    delayMicroseconds(300);
    int anlgMeasure = analogRead(analogPins[anlgIdx]);

    if (ctrlIdx < NB_DRAWBARS)
    {
      storeDrawbarPosition(anlgMeasure, ctrlIdx);
      onDrawbarMove(ctrlIdx);
    }
    else if (ctrlIdx == LESLIE_COMMON_IDX)
    {
      leslie_new = getLesliePosition(anlgMeasure);
      onLeslieSwitchMove();
    }
  }
}

void storeDrawbarPosition(int anlgMeasure, int ctrlIdx)
{
  if (anlgMeasure >= 0 && anlgMeasure < 50)
    pos_new[ctrlIdx] = 0;

  else if (anlgMeasure > 100 && anlgMeasure < 150)
    pos_new[ctrlIdx] = 1;

  else if (anlgMeasure > 220 && anlgMeasure < 280)
    pos_new[ctrlIdx] = 2;

  else if (anlgMeasure > 340 && anlgMeasure < 420)
    pos_new[ctrlIdx] = 3;

  else if (anlgMeasure > 430 && anlgMeasure < 530)
    pos_new[ctrlIdx] = 4;

  else if (anlgMeasure > 550 && anlgMeasure < 670)
    pos_new[ctrlIdx] = 5;

  else if (anlgMeasure > 700 && anlgMeasure < 790)
    pos_new[ctrlIdx] = 6;

  else if (anlgMeasure > 850 && anlgMeasure < 930)
    pos_new[ctrlIdx] = 7;

  else if (anlgMeasure > 990 && anlgMeasure < 1030)
    pos_new[ctrlIdx] = 8;
}

/*
   Prepares drawbars MIDI Control Change data, then sends the message to setBfree.
   The intermediate drawbar position between two contact bars produces a zero voltage
   which should not be managed as the 0 position, but should be filtered out.

   drawbarIdx:          an integer value in the [0-37] range, used as a drawbar index
                        we have 38 drawbars (4 x 9 + 2)
*/
void onDrawbarMove(int drawbarIdx)
{
  byte midiChnl = getMidiChannel(drawbarIdx);
  int dbarCode = getDrawbarCode(midiChnl, drawbarIdx);
  sendControlChange(midiChnl, dbarCode, dbar_pos[pos_new[drawbarIdx]]);
  pos_old[drawbarIdx] = pos_new[drawbarIdx];
}

byte getLesliePosition(int anlgMeasure)
{
  if (anlgMeasure >= 0 && anlgMeasure < 50)
    return LESLIE_SLOW;

  else if (anlgMeasure > 990 && anlgMeasure < 1030)
    return LESLIE_FAST;

  return LESLIE_STOP;
}


/*
   Builds a Leslie MIDI Program Change message and sends it.
*/
void onLeslieSwitchMove()
{
  sendProgramChange(0, leslie_new);
  leslie_old = leslie_new;
}

/*
  Reads the 5 analog inputs of the ATMEGA4809 for each mux value.
  If drawbar position has changed, sends MIDI control change message to the
  corresponding MIDI controller.

  mux : multiplexer value used to read one analog value among eight
        mux values are [0..7]
*/
void onControlsChange(byte mux)
{
  //                       -- UPPER --    BASS    --- LOWER ---
  // drawbars array index [0..8][9..17] [18..19] [20..28][29..37]
  int ctrlIdx = 0;

  // read the 5 analog pins sequentially
  for (byte anlgIdx = 0; anlgIdx <= 4; anlgIdx++)
  {
    ctrlIdx = anlgIdx * 8 + mux;
    delayMicroseconds(300);
    int anlgMeasure = analogRead(analogPins[anlgIdx]);

    if (ctrlIdx < NB_DRAWBARS)
    {
      storeDrawbarPosition(anlgMeasure, ctrlIdx);

      if (pos_new[ctrlIdx] != pos_old[ctrlIdx])
        onDrawbarMove(ctrlIdx);
    }

    else if (ctrlIdx == LESLIE_COMMON_IDX)
    {
      leslie_new = getLesliePosition(anlgMeasure);

      if (leslie_new != leslie_old)
        onLeslieSwitchMove();
    }
  }
}

/*
   Calculates a drawbar setBfree code from the MIDI channel and the drawbar index.

   midiChnl:    UPPER_1 | UPPER_2 | LOWER_1 | LOWER_2 | PEDAL
   dbarIdx:     index of the drawbar [0-37]
*/
int getDrawbarCode(byte midiChnl, int dbarIdx)
{
  if (midiChnl == UPPER_1 || midiChnl == UPPER_2)
    return 70 + (dbarIdx % 9);

  else if (midiChnl == PEDAL)
    return dbarIdx == 18 ? 70 : 72;

  else
    return 70 + ((dbarIdx - 20) % 9);
}

/*
   The returned value is not the actual MIDI channel but rather
   a code to be sent to the RPI UP JavaFX controller, based on
   the group the moving drawbar belongs to.

   drawbarIndex:    index of the moving drawbar
*/
byte getMidiChannel(int drawbarIndex)
{
  if (drawbarIndex >= 0 && drawbarIndex <= 8)
    return UPPER_1;

  else if (drawbarIndex >= 9 && drawbarIndex <= 17)
    return UPPER_2;

  else if (drawbarIndex == 18 || drawbarIndex == 19)
    return PEDAL;

  else if (drawbarIndex >= 20 && drawbarIndex <= 28)
    return LOWER_1;

  else
    return LOWER_2;
}


/*
  Sends a MIDI Control Change message.

   chnl     : MIDI channel (UPPER | PEDAL | LOWER)
   dbar     : drawbar code or controller
   pos      : drawbar position
*/
void sendControlChange(byte chnl, byte dbar, byte pos)
{
  byte bytes[3];
  bytes[0] = chnl;
  bytes[1] = dbar;
  bytes[2] = pos;
  Serial.write(bytes, 3);

  // printDebug(chnl, dbar, pos);
}

/*
  Sends a MIDI Program Change message.
  The first byte is a dummy one, necessary because the Raspberry PI serial reader
  blocks until it receives 3 bytes from the Arduino.

   chnl     : MIDI channel (UPPER | PEDAL | LOWER)
   prog     : program to be changed
*/
void sendProgramChange(byte chnl, byte prog)
{
  byte bytes[3];
  bytes[0] = 127;
  bytes[1] = chnl;
  bytes[2] = prog;
  Serial.write(bytes, 3);
}

void printDebug(byte chnl, byte dbar, byte pos)
{
  if (chnl == UPPER_1)
    Serial.print("UPPER_1 / drawbar code ");
  else if (chnl == LOWER_1)
    Serial.print("LOWER_1 / drawbar code ");
  else if (chnl == UPPER_2)
    Serial.print("UPPER_2 / drawbar code ");
  else if (chnl == LOWER_2)
    Serial.print("LOWER_2 / drawbar code ");
  else
    Serial.print("PEDAL / drawbar code ");

  Serial.print(dbar);
  Serial.print(" / drawbar position ");
  for (int i = 0; i < 9; i++)
  {
    if (dbar_pos[i] == pos)
    {
      Serial.println(i);
      break;
    }
  }

}
