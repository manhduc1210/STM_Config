import serial
import struct

PORT = "COM5"
BAUD = 115200

OTA_MAGIC = 0x4F54
OTA_VERSION = 0x01

CMD_ERASE_APP = 0x04

CMD_ACK = 0x80
CMD_NACK = 0x81

def send_frame(ser, cmd, seq, offset=0, payload=b""):
    header = struct.pack(
        "<HBBIIHI",
        OTA_MAGIC,
        OTA_VERSION,
        cmd,
        seq,
        offset,
        len(payload),
        0
    )

    ser.write(header + payload)

def read_resp(ser):
    header = ser.read(18)
    if len(header) != 18:
        print("No response")
        return

    magic, version, cmd, seq, offset, length, crc32 = struct.unpack("<HBBIIHI", header)
    payload = ser.read(length)

    print("magic :", hex(magic))
    print("cmd   :", hex(cmd))
    print("seq   :", seq)
    print("len   :", length)

    if length == 2:
        status, = struct.unpack("<H", payload)
        print("status:", hex(status))

with serial.Serial(PORT, BAUD, timeout=3) as ser:
    send_frame(ser, CMD_ERASE_APP, seq=1)
    read_resp(ser)