import serial
import struct
import time

PORT = "COM9"
BAUD = 115200
TIMEOUT = 20

OTA_MAGIC = 0x4F54
OTA_VERSION = 0x01

CMD_ERASE_APP = 0x04

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
    buf = bytearray()

    while True:
        b = ser.read(1)
        if not b:
            print("No response")
            return

        buf += b

        # OTA_MAGIC = 0x4F54, little-endian on UART: 54 4F
        if len(buf) >= 2 and buf[-2:] == b"\x54\x4F":
            header = bytearray(b"\x54\x4F")
            rest = ser.read(16)

            if len(rest) != 16:
                print("Incomplete response header")
                return

            header += rest
            break

    print("received bytes:", len(header))
    print("raw header    :", header.hex(" "))

    magic, version, cmd, seq, offset, length, crc32 = struct.unpack("<HBBIIHI", header)
    payload = ser.read(length)

    print("magic :", hex(magic))
    print("cmd   :", hex(cmd))
    print("seq   :", seq)
    print("len   :", length)

    if length == 2:
        if len(payload) != 2:
            print(f"Incomplete payload ({len(payload)}/2 bytes)")
            return

        status, = struct.unpack("<H", payload)
        print("status:", hex(status))

with serial.Serial(PORT, BAUD, timeout=TIMEOUT) as ser:
    time.sleep(2)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    send_frame(ser, CMD_ERASE_APP, seq=1)
    read_resp(ser)
