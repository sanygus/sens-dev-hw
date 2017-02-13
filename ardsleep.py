import sys, serial, json

try:
	stime = int(sys.argv[1])
	ard = serial.Serial(port='/dev/ttyACM0', baudrate = 9600, timeout = 5, writeTimeout = 5)
	ard.write(b'\x02' + stime.to_bytes(2, byteorder = 'big'))

	confirm = ard.read(1)
	if confirm[0] == 1:
		print(json.dumps({'success': True}))
	else:
		print(json.dumps({'success': False}))
except:
	print(json.dumps({'success': False}))
