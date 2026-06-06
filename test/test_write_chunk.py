import serial
import struct
import time

PORT = "COM9"
BAUD = 115200

OTA_MAGIC = 0x4F54
OTA_VERSION = 0x01

CMD_ERASE_APP = 0x04
CMD_WRITE_CHUNK = 0x05

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
            return False

        buf += b

        if len(buf) >= 2 and buf[-2:] == b"\x54\x4F":
            header = bytearray(b"\x54\x4F")
            rest = ser.read(16)

            if len(rest) != 16:
                print("Incomplete header")
                return False

            header += rest
            break

    magic, version, cmd, seq, offset, length, crc32 = struct.unpack("<HBBIIHI", header)
    payload = ser.read(length)

    status = None
    if length == 2:
        status, = struct.unpack("<H", payload)

    print(f"resp cmd={hex(cmd)} seq={seq} status={hex(status) if status is not None else None}")
    return cmd == 0x80 and status == 0

with serial.Serial(PORT, BAUD, timeout=3) as ser:
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    print("Erase app...")
    send_frame(ser, CMD_ERASE_APP, seq=1)
    assert read_resp(ser)

    time.sleep(0.2)

    chunk = bytes([
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC,
        0xDD, 0xEE, 0x12, 0x34
    ])

    print("Write chunk...")
    send_frame(ser, CMD_WRITE_CHUNK, seq=2, offset=0, payload=chunk)
    assert read_resp(ser)

    print("PASS")