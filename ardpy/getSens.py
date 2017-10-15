import smbus, time, json
res = {}

for i in range(5):
  res = {}
  try:
    bus = smbus.SMBus(1)
    # power up LPS331AP pressure sensor & set BDU bit
    # active mode
    bus.write_byte_data(0x5c, 0x20, 0b10000100)
    # reboot memory content
    bus.write_byte_data(0x5c,0x21, 0b1)
    # delay for write values to registers (enough 0.05, 0.1 for reliability)
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
    # power down
    #bus.write_byte_data(0x5c, 0x20, 0b00000100)
    # soft reboot
    #bus.write_byte_data(0x5c,0x21, 0b10000100)
    bus.close()
    if (res['temp'] > -50):
      break
  except IOError:
    res['error2'] = 'IOError'
  except:
    res['error2'] = 'unknown error in second block'
  time.sleep(1)

print(json.dumps(res, sort_keys=True))
#example {"temp": 27.2, "press": 742.7, "mic": 330, "volt": 312, "gas1": [4, 11, 20, 9], "gas2": [7, 10, 5]}

'''
try:
  bus.write_i2c_block_data(0x05, 1, [0, 0])
  data = bus.read_i2c_block_data(0x05, 0, 19)
  if data[0] == 1:
    if data[1]:
      res['gas1'] = [
        data[2] << 8 | data[3],
        data[4] << 8 | data[5],
        data[6] << 8 | data[7],
        data[8] << 8 | data[9]
      ]
    
    if data[10]:
      res['gas2'] = [
        data[11] << 8 | data[12],
        data[13] << 8 | data[14],
        data[15] << 8 | data[16]
      ]
    res['mic'] = data[17] << 8 | data[18]
  else:
    res['error1'] = 'unknown answer'
except IOError:
  res['error1'] = 'IOError'
except:
  res['error1'] = 'unknown error in first block'
'''
