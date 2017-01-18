import smbus, sys, json
bus = smbus.SMBus(1)

try:
	stime = int(sys.argv[1])
	bus.write_i2c_block_data(0x05, 1, [stime >> 8, stime & 0xFF])
	print(json.dumps({'success': True}))
except:
	print(json.dumps({'success': False}))
