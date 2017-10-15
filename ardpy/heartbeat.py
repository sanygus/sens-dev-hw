import smbus, time

while(True):
	try:
		bus = smbus.SMBus(1)
		bus.write_i2c_block_data(0x05, 11, [0, 0])
		data = bus.read_i2c_block_data(0x05, 0, 1)
		bus.close()
		if data[0] != 1:
			print('warning 1')
	except:
		print('warning 2')

	time.sleep(10)
