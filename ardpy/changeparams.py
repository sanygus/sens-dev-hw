import sys, smbus, json
bus = smbus.SMBus(1)

try:
	cmd = int(sys.argv[1])
	val = int(sys.argv[2])
	bus.write_i2c_block_data(0x05, cmd, [(val >> 8) & 0xFF, val & 0xFF])
	data = bus.read_i2c_block_data(0x05, 0, 1)
	if data[0] == 1:
		print(json.dumps({'success': True}))
	elif data[0] == 2:
		print(json.dumps({'success': False}))
except:
	print(json.dumps({'success': False}))
