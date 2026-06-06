import serial

ser = serial.Serial("COM9", 115200, timeout=3)
print(ser.read(100))