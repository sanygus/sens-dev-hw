import smbus, time, json
bus = smbus.SMBus(1)
res = {}

try:
  data = bus.read_i2c_block_data(0x05, 0, 20)

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
except IOError:
  res['error1'] = 'IOError'
except:
  res['error1'] = 'unknown error in first block'

try:
  # power up LPS331AP pressure sensor & set BDU bit
  bus.write_byte_data(0x5c, 0x20, 0b10000100)
  #write value 0b1 to register 0x21 on device at address 0x5d
  bus.write_byte_data(0x5c,0x21, 0b1)
  #delay for write values to registers (enough 0.05, 0.1 for reliability)
  time.sleep(0.1)

  Temp_LSB = bus.read_byte_data(0x5c, 0x2b)
  Temp_MSB = bus.read_byte_data(0x5c, 0x2c)
  #combine LSB & MSB
  count = (Temp_MSB << 8) | Temp_LSB
  # As value is negative convert 2's complement to decimal
  comp = count - (1 << 16)
  #calc temp according to data sheet
  res['temp'] = round((42.5 + (comp/480.0)), 1)

  Pressure_XLB = bus.read_byte_data(0x5c, 0x28)
  Pressure_LSB = bus.read_byte_data(0x5c, 0x29)
  Pressure_MSB = bus.read_byte_data(0x5c, 0x2a)
  count = (Pressure_MSB << 16) | ( Pressure_LSB << 8 ) | Pressure_XLB
  res['press'] = round(((count/4096.0)*0.75006), 1)
except IOError:
  res['error2'] = 'IOError'
except:
  res['error2'] = 'unknown error in second block'

print json.dumps(res)
#example {"temp": 27.2, "press": 742.7, "mic": 330, "volt": 312, "gas1": [4, 11, 20, 9], "gas2": [7, 10, 5]}
