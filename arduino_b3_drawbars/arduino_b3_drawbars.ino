/*************************************************************************
	SetBfree Harmonic Drawbars Control.

  From bass to treble pipes, drawbars names are the following:
  Controller indexes are decimal values

  Controller    Function
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

  idx    anlgIdx    mux
  =============================
		0          0       0       UPKB REG1 RV1
		1          0       1
		2          0       2
		3          0       3
		4          0       4
		5          0       5
		6          0       6
		7          0       7
		8          1       X       UPKB REG1 RV9

    9          2       0       UPKB REG2 RV1
   10          2       1
   11          2       2
   12          2       3
   13          2       4
   14          2       5
   15          2       6
   16          2       7
   17          3       0       UPKB REG2 RV9
   18          3       1       PDKB      RV10
   19          3       2       PDKB      RV11

   20          4       0       LOKB REG1 RV1
   21          4       1
   22          4       2
   23          4       3
   24          4       4
   25          4       5
   26          4       6
   27          4       7
   28          5       X       LOKB REG1 RV9

   29          6       0       LOKB REG2 RV1
   30          6       1
   31          6       2
   32          6       3
   33          6       4
   34          6       5
   35          6       6
   36          6       7
   37          7       X       LOKB REG2 RV9
*/

// used as MIDI channel value in Control Change messages
struct midiChannel {
    byte UPPER_A = 0;
    byte UPPER_B = 0;
    byte LOWER_A = 1;
    byte LOWER_B = 1;
    byte BASS = 2;
};
midiChannel mc;

// a value out of the possible values range, used to initialize pos_old and pos_new arrays
// so that whatever the first drawbar position measurement is, we will send a CC message
#define DRAWBAR_POS_INIT 255

// sent by RPI to Arduino to change drawbars settings
struct rpiCommand {

    char UPPER_0[3] = "U0";
    char UPPER_1[3] = "U1";
    char UPPER_2[3] = "U2";
    char UPPER_3[3] = "U3";
    char UPPER_4[3] = "U4";
    char UPPER_5[3] = "U5";
    char UPPER_A[3] = "UA";
    char UPPER_B[3] = "UB";
    char LOWER_0[3] = "L0";
    char LOWER_1[3] = "L1";
    char LOWER_2[3] = "L2";
    char LOWER_3[3] = "L3";
    char LOWER_4[3] = "L4";
    char LOWER_5[3] = "L5";
    char LOWER_A[3] = "LA";
    char LOWER_B[3] = "LB";

    char BASS[2] = "B";
    byte ETB = 23;                                // ASCII End of Transmission Block
    char DRAWBARS_ARDUINO_IDENTIFIER[4] = "DAI";  // sent by RPI to all serial ports to identify the drawbars Arduino
    char DRAWBARS_ARDUINO_ACK[3] = "OK";          // sent to the RPI to acknowledge its commands
    char END_OF_COMMAND[1] = { '\n' };
};
rpiCommand rc;


// multiplexers controls (see Drawbars Control Board KiCad schematics)
const int MUX_A = 2;  // D2
const int MUX_B = 3;  // D3
const int MUX_C = 4;  // D4

// selected registration LEDs controls
const int UP_REG_LED = 5;  // D5
const int LO_REG_LED = 6;  // D6

// analog reading with average calculation
int analogPins[8] = { A0, A1, A2, A3, A4, A5, A6, A7 };

#define NB_DRAWBARS 38  // 4 x 9 + 2

// we have NB_DRAWBARS drawbars and every one has 9 possible positions, which can be coded on one byte
// we send a MIDI message on initialization, or when a drawbar position has changed
byte pos_old[NB_DRAWBARS];
byte pos_new[NB_DRAWBARS];

//                    0    1   2   3   4   5   6   7   8
byte dbar_pos[9] = { 127, 110, 92, 79, 63, 47, 31, 15, 0 };

// active registrations flags
// DO NOT INITIALIZE 'a' FLAGS TRUE, OTHERWISE THE ARDUINO WILL START SENDING
// MIDI CC MESSAGES WHILE THE RPI IS TRYING TO IDENTIFY THE DRAWBARS ARDUINO
bool reg_up_a = false;
bool reg_up_b = false;
bool reg_lo_a = false;
bool reg_lo_b = false;

/*
   Arduino program setup. This function is executed only once.
*/
void setup() {

    setupCtrlPins();

    for (int i = 0; i < NB_DRAWBARS; i++) {
        pos_old[i] = DRAWBAR_POS_INIT;
        pos_new[i] = DRAWBAR_POS_INIT;
    }

    analogReference(EXTERNAL);  // Vdd of the ATmega4809
    Serial.begin(115200);

    while (!Serial) {
        ;  // wait for serial port to connect. Needed for native USB port only
    }

    // the very first call to analogRead() after powering up returns junk;
    // this is a documented issue with the ATmega chips.
    analogRead(analogPins[0]);
}

void setupCtrlPins(void) {

    pinMode(LED_BUILTIN, OUTPUT);

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

    // unused pins
    pinMode(7, INPUT_PULLUP);
    pinMode(8, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(11, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);

    digitalWrite(MUX_A, LOW);
    digitalWrite(MUX_B, LOW);
    digitalWrite(MUX_C, LOW);

    digitalWrite(UP_REG_LED, LOW);
    digitalWrite(LO_REG_LED, LOW);
}

/*
  Endless Arduino program main loop.
*/
void loop() {

    static unsigned long time = 0;

    if (time == 0)
        time = millis();

    unsigned long curTime = millis();

    // if 500ms have elapsed, read all drawbars again
    if (curTime > time + 500) {
        readDrawbarsPositions();
        time = curTime;

        executeRaspberryPiCommand();
    }
}

/*
  The Raspberry PI sends commands to the Arduinos whenever it wants  
  to change their settings. This function is called repeatidly to check
  for command codes sent by the RPI through the serial port.
  If there is any command available, it is read and executed.
*/
void executeRaspberryPiCommand(void) {

    if (Serial.available() > 0) {

        String cmd = Serial.readStringUntil('\n');

        if (cmd == rc.UPPER_A) {
            pinMode(UP_REG_LED, OUTPUT);
            digitalWrite(UP_REG_LED, HIGH);
            reg_up_a = true;
            reg_up_b = false;
            sendDrawbarsPositions(cmd);
        }

        else if (cmd == rc.UPPER_B) {
            pinMode(UP_REG_LED, OUTPUT);
            digitalWrite(UP_REG_LED, LOW);
            reg_up_a = false;
            reg_up_b = true;
            sendDrawbarsPositions(cmd);
        }

        else if (cmd == rc.LOWER_A) {
            pinMode(LO_REG_LED, OUTPUT);
            digitalWrite(LO_REG_LED, HIGH);
            reg_lo_a = true;
            reg_lo_b = false;
            sendDrawbarsPositions(cmd);
        }

        else if (cmd == rc.LOWER_B) {
            pinMode(LO_REG_LED, OUTPUT);
            digitalWrite(LO_REG_LED, LOW);
            reg_lo_a = false;
            reg_lo_b = true;
            sendDrawbarsPositions(cmd);
        }

        else if (cmd == rc.BASS) {
            sendDrawbarsPositions(cmd);
        }

        else if (cmd == rc.DRAWBARS_ARDUINO_IDENTIFIER) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(200);
            digitalWrite(LED_BUILTIN, LOW);

            Serial.write(rc.DRAWBARS_ARDUINO_ACK);  // send this String as a series of bytes
        }
        
        else if (cmd == rc.UPPER_0 || cmd == rc.UPPER_1 || cmd == rc.UPPER_2 || cmd == rc.UPPER_3 || cmd == rc.UPPER_4 || cmd == rc.UPPER_5) {
            reg_up_a = false;
            reg_up_b = false;
        }

        else if (cmd == rc.LOWER_0 || cmd == rc.LOWER_1 || cmd == rc.LOWER_2 || cmd == rc.LOWER_3 || cmd == rc.LOWER_4 || cmd == rc.LOWER_5) {
            reg_lo_a = false;
            reg_lo_b = false;
        }
    }
}


void selectDrawbar(int mux) {

    digitalWrite(MUX_A, mux & 0x01 ? HIGH : LOW);
    digitalWrite(MUX_B, mux & 0x02 ? HIGH : LOW);
    digitalWrite(MUX_C, mux & 0x04 ? HIGH : LOW);
}

/*
  Drawbars positions have to be sent to setBfree on initialization as well as
  on registration change.
  On initialization: upper A, lower A and bass drawbars settings are sent.
  On registration change: drawbars settings of the selected registration are sent.

  cmd: String command sent by the RPI
*/
void sendDrawbarsPositions(String cmd) {

    if (cmd == rc.UPPER_A) {
        for (int idx = 0; idx <= 8; idx++) {
            onDrawbarMove(idx);
        }
    }

    else if (cmd == rc.UPPER_B) {
        for (int idx = 9; idx <= 17; idx++) {
            onDrawbarMove(idx);
        }
    }

    else if (cmd == rc.BASS) {
        for (int idx = 18; idx <= 19; idx++) {
            onDrawbarMove(idx);
        }
    }

    else if (cmd == rc.LOWER_A) {
        for (int idx = 20; idx <= 28; idx++) {
            onDrawbarMove(idx);
        }
    }

    else if (cmd == rc.LOWER_B) {
        for (int idx = 29; idx <= 37; idx++) {
            onDrawbarMove(idx);
        }
    }
}

/*
  Reads drawbars positions, then sends MIDI Control Change messages to
  the corresponding MIDI controller.
*/
void readDrawbarsPositions() {

    int idx = 0;  // drawbar index
    int anlgMeasure = 0;

    //                       -- UPPER --    BASS    --- LOWER ---
    // drawbars array index [0..8][9..17] [18..19] [20..28][29..37]

    // =================================  Board 1  ==================================

    for (int mux = 0; mux <= 7; mux++) {

        selectDrawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[0]);
        storeDrawbarPosition(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            onDrawbarMove(idx);

        idx++;
    }

    delay(10);  // ms
    anlgMeasure = analogRead(analogPins[1]);
    storeDrawbarPosition(anlgMeasure, idx);

    if (pos_new[idx] != pos_old[idx])
        onDrawbarMove(idx);

    idx++;

    // =================================  Board 2  ==================================

    for (int mux = 0; mux <= 7; mux++) {

        selectDrawbar(mux);
        delay(10);  //ms
        anlgMeasure = analogRead(analogPins[2]);
        storeDrawbarPosition(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            onDrawbarMove(idx);

        idx++;
    }

    for (int mux = 0; mux <= 2; mux++) {

        selectDrawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[3]);
        storeDrawbarPosition(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            onDrawbarMove(idx);

        idx++;
    }

    // =================================  Board 3  ==================================
    
    for (int mux = 0; mux <= 7; mux++) {

        selectDrawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[4]);
        storeDrawbarPosition(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            onDrawbarMove(idx);

        idx++;
    }

    delay(10);  // ms
    anlgMeasure = analogRead(analogPins[5]);
    storeDrawbarPosition(anlgMeasure, idx);

    if (pos_new[idx] != pos_old[idx])
        onDrawbarMove(idx);

    idx++;

    // =================================  Board 4  ==================================

    for (int mux = 0; mux <= 7; mux++) {

        selectDrawbar(mux);
        delay(10);  // ms
        anlgMeasure = analogRead(analogPins[6]);
        storeDrawbarPosition(anlgMeasure, idx);

        if (pos_new[idx] != pos_old[idx])
            onDrawbarMove(idx);

        idx++;
    }

    delay(10);  // ms
    anlgMeasure = analogRead(analogPins[7]);
    storeDrawbarPosition(anlgMeasure, idx);

    if (pos_new[idx] != pos_old[idx])
        onDrawbarMove(idx);
}

/*
  Stores a drawbar position (0..8). It will be compared to the
  previous position to check if the drawbar position has changed.

	anlgMeasure:    analog value measured on the drawbar
	idx:            index of the drawbar [0..37]
*/
void storeDrawbarPosition(int anlgMeasure, int idx) {

    if (anlgMeasure >= 0 && anlgMeasure <= 63)
        pos_new[idx] = 0;

    else if (anlgMeasure >= 64 && anlgMeasure <= 191)
        pos_new[idx] = 1;

    else if (anlgMeasure >= 192 && anlgMeasure <= 319)
        pos_new[idx] = 2;

    else if (anlgMeasure >= 320 && anlgMeasure <= 447)
        pos_new[idx] = 3;

    else if (anlgMeasure >= 448 && anlgMeasure <= 575)
        pos_new[idx] = 4;

    else if (anlgMeasure >= 576 && anlgMeasure <= 703)
        pos_new[idx] = 5;

    else if (anlgMeasure >= 704 && anlgMeasure <= 831)
        pos_new[idx] = 6;

    else if (anlgMeasure >= 832 && anlgMeasure <= 959)
        pos_new[idx] = 7;

    else if (anlgMeasure >= 960 && anlgMeasure <= 1023)
        pos_new[idx] = 8;
}

/*
  Prepares drawbars MIDI Control Change data, then sends the message to setBfree.
  MIDI CC messages are sent if and only if the drawbar index matches the selected
  keyboard registration (excepted for bass).

  idx: index of the moving drawbar in the [0..37] range
*/
void onDrawbarMove(int idx) {

    byte midiChnl = getMidiChannel(idx);
    int midiCtrl = getMidiController(idx);

    if ((idx >= 0 && idx <= 8 && reg_up_a) || (idx >= 9 && idx <= 17 && reg_up_b) || (idx == 18 || idx == 19) || (idx >= 20 && idx <= 28 && reg_lo_a) || (idx >= 29 && idx <= 37 && reg_lo_b))
        sendControlChange(midiChnl, midiCtrl, dbar_pos[pos_new[idx]]);

    pos_old[idx] = pos_new[idx];
}

/*
  Upper keyboard drawbars moves produce MIDI CC messages on channel 0.
  Lower keyboard drawbars moves produce MIDI CC messages on channel 1.
  Pedal board drawbars moves produce MIDI CC messages on channel 2.
	
	idx:  index of the moving drawbar
*/
byte getMidiChannel(int idx) {

    if (idx >= 0 && idx <= 8)
        return mc.UPPER_A;

    else if (idx >= 9 && idx <= 17)
        return mc.UPPER_B;

    else if (idx == 18 || idx == 19)
        return mc.BASS;

    else if (idx >= 20 && idx <= 28)
        return mc.LOWER_A;

    else
        return mc.LOWER_B;
}

/*
  Calculates and returns a drawbar MIDI controller index.

	idx:  index of the drawbar [0-37]
*/
int getMidiController(int idx) {

    if (idx >= 0 && idx <= 17)
        return 70 + (idx % 9);

    else if (idx == 18)
        return 70;

    else if (idx == 19)
        return 72;

    else
        return 70 + ((idx - 20) % 9);
}

/*
  Sends MIDI CC messages.

	channel:    MIDI channel index
	controller: MIDI controller value
	value:      value byte to be sent as CC
*/
void sendControlChange(byte channel, byte controller, byte value) {

    byte bytes[4];

    bytes[0] = 0xB0 | channel;
    bytes[1] = controller;
    bytes[2] = value;
    bytes[3] = rc.ETB;

    Serial.write(bytes, 4);
}

void printDebug(byte chnl, byte dbar, byte pos) {

    if (chnl == mc.UPPER_A)
        Serial.print("UPPER_A / drawbar code ");
    else if (chnl == mc.LOWER_A)
        Serial.print("LOWER_A / drawbar code ");
    else if (chnl == mc.UPPER_B)
        Serial.print("UPPER_B / drawbar code ");
    else if (chnl == mc.LOWER_B)
        Serial.print("LOWER_B / drawbar code ");
    else
        Serial.print("PEDAL / drawbar code ");

    Serial.print(dbar);
    Serial.print(" / drawbar position ");
    for (int i = 0; i < 9; i++) {
        if (dbar_pos[i] == pos) {
            Serial.println(i);
            break;
        }
    }
}
