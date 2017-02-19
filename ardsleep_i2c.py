import sys, smbus, json
bus = smbus.SMBus(1)

try:
	stime = int(sys.argv[1])
	bus.write_i2c_block_data(0x05, 2, [stime >> 8, stime & 0xFF])
	data = bus.read_i2c_block_data(0x05, 0, 1)
	if data[0] == 1:
		print(json.dumps({'success': True}))
	else if data[0] == 2:
		print(json.dumps({'success': False}))
		print('ok hand err')
except:
	print(json.dumps({'success': False}))
