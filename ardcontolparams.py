import smbus
from pprint import pprint
bus = smbus.SMBus(1)

try:
	bus.write_i2c_block_data(0x05, 10, [0, 0])
	data = bus.read_i2c_block_data(0x05, 0, 10)
	res = {}
	res[5] = data[0] << 8 | data[1]#mincharge
	res[6] = data[2] << 8 | data[3]#maxcharge
	res[7] = data[4] << 8 | data[5]#devid
	res[8] = data[6] << 8 | data[7]#port
	res[9] = data[8] << 8 | data[9]#wakethres
	pprint(res)
except:
	print("error")