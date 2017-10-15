import smbus, time, json
res = {}

for i in range(5):
  res = {}
  try:
    bus = smbus.SMBus(1)
    bus.write_i2c_block_data(0x05, 3, [0, 0])
    data = bus.read_i2c_block_data(0x05, 0, 3)
    bus.close()
    if data[0] == 1:
      res['charge'] = round((data[1] << 8 | data[2]) / 1000, 3)
      break
    else:
      res['error'] = 'get charge err'
  except IOError:
    res['error'] = 'IOError'
  except:
    res['error'] = 'unknown error'
  time.sleep(1)

print(json.dumps(res, sort_keys=True))
