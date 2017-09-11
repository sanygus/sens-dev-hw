import smbus, time, json
bus = smbus.SMBus(1)
res = {}

try:
  bus.write_i2c_block_data(0x05, 3, [0, 0])
  data = bus.read_i2c_block_data(0x05, 0, 3)
  if data[0]:
    res['charge'] = round((data[1] << 8 | data[2]) / 1000, 3)
  else:
    res['error'] = 'get charge err'
except IOError:
  res['error'] = 'IOError'
except:
  res['error'] = 'unknown error'

bus.close()
print(json.dumps(res, sort_keys=True))
