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
        0           0       0       UPKB REG1 RV1
        1           0       1
        2           0       2
        3           0       3
        4           0       4
        5           0       5
        6           0       6
        7           0       7
        8           1       X       UPKB REG1 RV9

        9           2       0       UPKB REG2 RV1
       10           2       1
       11           2       2
       12           2       3
       13           2       4
       14           2       5
       15           2       6
       16           2       7
       17           3       0       UPKB REG2 RV9
       18           3       1       PDKB      RV10
       19           3       2       PDKB      RV11

       20           4       0       LOKB REG1 RV1
       21           4       1
       22           4       2
       23           4       3
       24           4       4
       25           4       5
       26           4       6
       27           4       7
       28           5       X       LOKB REG1 RV9

       29           6       0       LOKB REG2 RV1
       30           6       1
       31           6       2
       32           6       3
       33           6       4
       34           6       5
       35           6       6
       36           6       7
       37           7       X       LOKB REG2 RV9
*/

// MIDI channels
// Drawbars moves messages are not sent directly as MIDI messages to setBfree
// because we have two sets of drawbars per manual keyboard and therefore we
// want to know which set is in use. For that purpose, I assign temporary channels
// to drawbars sets and the RPI Python controller will manage the actual MIDI
// channel assignment to be sent to setBfree.
#define UPPER_1 0
#define UPPER_2 3
#define LOWER_1 1
#define LOWER_2 4
#define PEDAL 2

#define NB_DRAWBARS 38
#define ACK 0x06

#define CTRL_INIT 127

// multiplexers controls
const int  MUX_A = 2; // D2
const int  MUX_B = 3; // D3
const int  MUX_C = 4; // D4

// selected registration LEDs controls
const int UP_REG_LED = 5; // D5
const int LO_REG_LED = 6; // D6

// analog reading with average calculation
int analogPins[8] = {A0, A1, A2, A3, A4, A5, A6, A7};

// we have NB_DRAWBARS drawbars and every one has 9 possible positions, which can be coded on one byte
// we send a MIDI message only when a drawbar position has changed
byte pos_old[NB_DRAWBARS];
byte pos_new[NB_DRAWBARS];

//                    0    1   2   3   4   5   6   7  8
byte dbar_pos[9] = {127, 110, 92, 79, 63, 47, 31, 15, 0};

const bool INITIALIZING = true;
const bool CHECKING = false;

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

  // the very first call to analogRead() after powering up returns junk,
  // this is a documented issue with the ATmega chips.
  int anlgMeasure = analogRead(analogPins[0]);
  
  sendControlsPositions(INITIALIZING);
}

/*
   Arduino program main loop.
*/
void loop()
{
  static unsigned long time = 0;

  if (time == 0)
    time = millis();

  unsigned long curTime = millis();

  // if 500ms have elapsed, read all drawbars again
  if (curTime > time + 500)
  {
    sendControlsPositions(CHECKING);
    time = curTime;

    readRaspberryPiCommand();
  }
}

void readRaspberryPiCommand(void)
{
  if (Serial.available() > 0)
  {
    String cmd = Serial.readStringUntil('\n');
    
    if (cmd == "1")
    {
      digitalWrite(UP_REG_LED, HIGH);
      digitalWrite(LO_REG_LED, HIGH);
    }

    else if (cmd == "2")
    {
      digitalWrite(UP_REG_LED, LOW);
      digitalWrite(LO_REG_LED, LOW);
    }
  }
}

void setupCtrlPins(void)
{
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);

  pinMode(UP_REG_LED, OUTPUT);
  pinMode(LO_REG_LED, OUTPUT);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(A6, INPUT);
  pinMode(A7, INPUT);  

  digitalWrite(MUX_A, LOW);
  digitalWrite(MUX_B, LOW);
  digitalWrite(MUX_C, LOW);

  digitalWrite(UP_REG_LED, LOW);
  digitalWrite(LO_REG_LED, LOW);

  for (int i = 0; i < NB_DRAWBARS; i++)
  {
    pos_old[i] = CTRL_INIT;
    pos_new[i] = CTRL_INIT;
  }
}

/*
  We are reading drawbars initial positions and therefore we send them
  without checking for positions changes.
  Reads the 8 analog inputs of the ATMEGA4809 for each mux value.
  Sends MIDI control change message to the corresponding MIDI controller.
*/
void sendControlsPositions(bool initializing)
{
  int ctrlIdx = 0;
  int anlgMeasure = 0;

  //                       -- UPPER --    BASS    --- LOWER ---
  // drawbars array index [0..8][9..17] [18..19] [20..28][29..37]

  // =================================  Board 1  ==================================
  
  for (int mux = 0; mux <= 7; mux++)
  {
    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);

    delay(10); // 10ms
    anlgMeasure = analogRead(analogPins[0]);
    storeDrawbarPosition(anlgMeasure, ctrlIdx);

    if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
      onDrawbarMove(ctrlIdx);

    ctrlIdx++;      
  }

  delay(10);
  anlgMeasure = analogRead(analogPins[1]);
  storeDrawbarPosition(anlgMeasure, ctrlIdx);

  if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
    onDrawbarMove(ctrlIdx);

  ctrlIdx++;
  return;  
  // =================================  Board 2  ==================================

  for (int mux = 0; mux <= 7; mux++)
  {
    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);

    delay(10);
    anlgMeasure = analogRead(analogPins[2]);
    storeDrawbarPosition(anlgMeasure, ctrlIdx);

    if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
      onDrawbarMove(ctrlIdx);

    ctrlIdx++;
  }

  for (int mux = 0; mux <= 2; mux++)
  {
    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);

    delay(10);
    anlgMeasure = analogRead(analogPins[3]);
    storeDrawbarPosition(anlgMeasure, ctrlIdx);

    if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
      onDrawbarMove(ctrlIdx);

    ctrlIdx++;
  } 

  // =================================  Board 3  ==================================

  for (int mux = 0; mux <= 7; mux++)
  {
    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);

    delay(10);
    anlgMeasure = analogRead(analogPins[4]);
    storeDrawbarPosition(anlgMeasure, ctrlIdx);

    if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
      onDrawbarMove(ctrlIdx);

    ctrlIdx++;      
  }

  delay(10);
  anlgMeasure = analogRead(analogPins[5]);
  storeDrawbarPosition(anlgMeasure, ctrlIdx);

  if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
    onDrawbarMove(ctrlIdx);

  ctrlIdx++;

  // =================================  Board 4  ==================================

  for (int mux = 0; mux <= 7; mux++)
  {
    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);

    delay(10);
    anlgMeasure = analogRead(analogPins[6]);
    storeDrawbarPosition(anlgMeasure, ctrlIdx);

    if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
      onDrawbarMove(ctrlIdx);

    ctrlIdx++;
  }

  delay(10);
  anlgMeasure = analogRead(analogPins[7]);
  storeDrawbarPosition(anlgMeasure, ctrlIdx);

  if ( (pos_new[ctrlIdx] != pos_old[ctrlIdx]) || initializing )
    onDrawbarMove(ctrlIdx);
}

void storeDrawbarPosition(int anlgMeasure, int ctrlIdx)
{
  if (anlgMeasure >= 0 && anlgMeasure <= 63)
    pos_new[ctrlIdx] = 0;

  else if (anlgMeasure >= 64 && anlgMeasure <= 191)
    pos_new[ctrlIdx] = 1;

  else if (anlgMeasure >= 192 && anlgMeasure <= 319)
    pos_new[ctrlIdx] = 2;

  else if (anlgMeasure >= 320 && anlgMeasure <= 447)
    pos_new[ctrlIdx] = 3;

  else if (anlgMeasure >= 448 && anlgMeasure <= 575)
    pos_new[ctrlIdx] = 4;

  else if (anlgMeasure >= 576 && anlgMeasure <= 703)
    pos_new[ctrlIdx] = 5;

  else if (anlgMeasure >= 704 && anlgMeasure <= 831)
    pos_new[ctrlIdx] = 6;

  else if (anlgMeasure >= 832 && anlgMeasure <= 959)
    pos_new[ctrlIdx] = 7;

  else if (anlgMeasure >= 960 && anlgMeasure <= 1023)
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
Sends MIDI Control Change messages with a trailing Line Feed.
This is not a regular MIDI CC message.
The LF character ensures a correct message detection on the RPI side which reads data asynchronously.
These pseudo MIDI CC messages are not sent to setBfree directly. They are sent to the
Python code running on the RPI which then builds the actual MIDI message and
sends it to setBfree.
*/
void sendControlChange(byte channel, byte control, byte value)
{
    byte bytes[4];
    bytes[0] = 0xB0 | channel;
    bytes[1] = control;
    bytes[2] = value;
    bytes[3] = 0x0A; // new line
    Serial.write(bytes, 4);
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
