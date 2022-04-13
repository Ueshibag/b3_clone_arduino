#define ACK 0x06

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    syncRPI();  // send a byte to synchronize until RPI replies
}

void loop()
{
    // put your main code here, to run repeatedly:
    for (int dbar = 0; dbar < 9; dbar++)
    {
        for (int pos = 0; pos < 9; pos++)
        {
            sendControlChange(0, 70 + dbar, pos);
        }
    }
}

void sendControlChange(byte chnl, byte dbar, byte pos)
{
  byte bytes[3];

  bytes[0] = 0xB0 | chnl;
  bytes[1] = dbar;
  bytes[2] = pos;
    
  Serial.write(bytes, 3);

  // wait for computer ACK
  while (Serial.available() <= 0)
  {
    delay(50);
  }
  int rcv = Serial.read();
  if (rcv == ACK)
    return;
}

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
