# Sends 0 then 1 repeateadly to the Arduino USB serial port,
# with 1s delay between sendings. Never stops.

import serial
import time

port = serial.Serial("/dev/ttyACM0", 115200, timeout=1.0)
time.sleep(3)

while True:
    port.write('0'.encode('utf-8'))
    print('0 sent')
    time.sleep(1)
    port.write('1'.encode('utf-8'))
    print('1 sent')
    time.sleep(1)
