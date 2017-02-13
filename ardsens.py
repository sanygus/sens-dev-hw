import serial, json
res = {}

try:
  ard = serial.Serial(port='/dev/ttyACM0', baudrate=9600, timeout=5, writeTimeout=5);
  ard.write(b'\x01\x00\x00')
  confirm = ard.read(1)
  if confirm[0] == 1:
    data = ard.read(24)
    if data[0]:
      res['gas1'] = [
        data[1] << 8 | data[2],
        data[3] << 8 | data[4],
        data[5] << 8 | data[6],
        data[7] << 8 | data[8]
      ]
    if data[9]:
      res['gas2'] = [
        data[10] << 8 | data[11],
        data[12] << 8 | data[13],
        data[14] << 8 | data[15]
      ]
    res['mic'] = data[16] << 8 | data[17]
    res['volt'] = data[18] << 8 | data[19]
    res['press'] = data[20] << 8 | data[21]
    res['temp'] = round(((data[22] << 8 | data[23]) / 100) - 50, 2)
  elif confirm[0] == 2:
    res['error'] = "ard not understand"
    ard.flushInput()
  else:
    res['error'] = "worng answer from ard"
    ard.flushInput()

except OSError:
  res['error'] = "ard connect error"
except:
  res['error'] = "ard other error"

print(json.dumps(res, sort_keys=True))
