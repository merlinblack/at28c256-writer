#!/usr/bin/env python

import serial

def doCommand(ser: serial.Serial, cmd):
    cmd = cmd + "\n";
    ser.write(cmd.encode())
    while(not ser.in_waiting):
        pass
    output = ''
    while(output != "completed.\r\n"):
        output = ser.readline().decode()
        print(output)

def doRead(ser: serial.Serial, filename):
    ser.write("r\n".encode())
    while(not ser.in_waiting):
        pass
    output = b''
    with open(filename, mode='wb') as fp:
        while(output.decode() != "completed.\r\n"):
            output = ser.readline()
            print(output.decode())
            if len(output)>4 and output[4] == 58: # ASCII ':'
                codedBytes = output[5:-2]
                value = 0
                second = False
                bytesOut = bytearray()
                for byte in codedBytes:
                    if byte < 65: # ASCII 'A'
                        value += byte - 48 # ASCII '0'
                    else:
                        value += byte - 55 # ASCII 'A' - 10
                    if second:
                        bytesOut.append(value)
                        second = False
                        value = 0
                    else:
                        value *= 0x10
                        second = True
                fp.write(bytesOut)


def main():
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
    while(not ser.in_waiting):
        pass
    while(ser.in_waiting):
        print(ser.readline().decode())
    doCommand(ser, "a0000")
    doCommand(ser, "s8000")
    doRead(ser, 'dump.bin')
    
    print('Done')

if __name__ == '__main__':
    main()
