
char b;

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  // initialize serial:
  Serial.begin(115200);

  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}

void loop() {

  if(Serial.available() > 0) // If there is any data in the serial port
  {
    b = Serial.read(); // Read the data    
    if (b == '0') {
      digitalWrite(LED_BUILTIN, LOW);
    }
    else
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
}
