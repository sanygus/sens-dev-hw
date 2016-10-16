import smbus, json
bus = smbus.SMBus(1)

try:
	data = bus.read_i2c_block_data(0x05,0,18)
	gas1 = []
	gas2 = []

	if data[0]:
		gas1 = [
			data[1] << 8 | data[2],
			data[3] << 8 | data[4],
			data[5] << 8 | data[6],
			data[7] << 8 | data[8]
		]
	
	if data[9]:
                gas2 = [
                        data[10] << 8 | data[11],
                        data[12] << 8 | data[13],
                        data[14] << 8 | data[15]
                ]
        
	print json.dumps({'gas1': gas1, 'gas2': gas2, 'mic': data[16] << 8 | data[17]})
except IOError:
	print json.dumps({'error': 'IOError'})
except:
	print json.dumps({'error': 'unknown error'})
