import sys, smbus, time, json
res = {}
stime = int(sys.argv[1])
for i in range(5):
  res = {}
  try:
    bus = smbus.SMBus(1)
    bus.write_i2c_block_data(0x05, 2, [(stime >> 8) & 0xFF, stime & 0xFF])
    data = bus.read_i2c_block_data(0x05, 0, 1)
    bus.close()
    if data[0] == 1:
      res = {'success': True}
      break
    elif data[0] == 2:
      res = {'success': False}
  except:
    res = {'success': False}
  time.sleep(1)

print(json.dumps(res))
