#!/usr/bin/env python

import serial;

def doCommand(ser: serial.Serial, cmd):
	cmd = cmd + "\n";
	ser.write(cmd.encode())
	while(not ser.in_waiting):
		pass
	output = ''
	while(output != "completed.\r\n"):
		output = ser.readline().decode()
		print(output)

def main():
	ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
	while(not ser.in_waiting):
		pass
	while(ser.in_waiting):
		print(ser.readline().decode())
	doCommand(ser, "a1000")
	doCommand(ser, "s20")
	for n in range(20):
		doCommand(ser, "w1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef")
	
	print('Done')

if __name__ == '__main__':
	main()
