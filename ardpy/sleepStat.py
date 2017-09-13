import smbus, time, json
bus = smbus.SMBus(1)
res = {}

try:
  bus.write_i2c_block_data(0x05, 4, [0, 0])
  data = bus.read_i2c_block_data(0x05, 0, 4)
  if data[0] == 1:
  	res['wakeup_reason'] = data[1]
  	res['conn_error'] = data[2]
  	res['modem_err'] = data[3]
  else:
    res['error'] = 'unknown answer'
except IOError:
  res['error'] = 'IOError'
except:
  res['error'] = 'unknown error'

bus.close()
print(json.dumps(res, sort_keys=True))
