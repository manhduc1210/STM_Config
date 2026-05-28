import serial
import struct

PORT = "COM9"
BAUD = 115200

OTA_MAGIC = 0x4F54
OTA_VERSION = 0x01
CMD_HELLO = 0x55

frame = struct.pack(
    "<HBBIHI",
    OTA_MAGIC,
    OTA_VERSION,
    CMD_HELLO,
    1,      # sequence
    0,      # length
    0       # crc32
)

ser = serial.Serial(PORT, BAUD, timeout=2)
ser.write(frame)

resp = ser.read(14)
print("RAW:", resp)

if len(resp) == 14:
    magic, version, cmd, seq, length, crc32 = struct.unpack("<HBBIHI", resp)
    print("magic  :", hex(magic))
    print("version:", version)
    print("cmd    :", hex(cmd))
    print("seq    :", seq)
    print("length :", length)
    print("crc32  :", crc32)
else:
    print("No valid response")