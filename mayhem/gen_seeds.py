#!/usr/bin/env python3
"""Regenerates the committed seed corpora under mayhem/<target>/testsuite/ (run from anywhere)."""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))


def cfg(ifaces, body):
    total = 9 + len(body)
    return struct.pack("<BBHBBBBB", 9, 2, total, ifaces, 1, 0, 0x80, 50) + body


EP = lambda addr, attr, mps, itv=0: struct.pack("<BBBBHB", 7, 5, addr, attr, mps, itv)
IF = lambda num, alt, neps, cls, sub, proto: struct.pack("<BBBBBBBBB", 9, 4, num, alt, neps, cls, sub, proto, 0)

MSC = cfg(1, IF(0, 0, 2, 8, 6, 0x50) + EP(0x81, 2, 512) + EP(0x02, 2, 512))
REPORT = bytes([0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03,
                0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0xC0, 0xC0])
HID = cfg(1, IF(0, 0, 1, 3, 1, 2) + struct.pack("<BBHBBBH", 9, 0x21, 0x0111, 0, 1, 0x22, len(REPORT)) + EP(0x81, 3, 4, 10))
CDC = cfg(2, bytes([8, 0x0B, 0, 2, 2, 2, 0, 0]) + IF(0, 0, 1, 2, 2, 0) +
          bytes([5, 0x24, 0, 0x10, 1, 5, 0x24, 1, 0, 1, 4, 0x24, 2, 2, 5, 0x24, 6, 0, 1]) + EP(0x83, 3, 8, 10) +
          IF(1, 0, 2, 0x0A, 0, 0) + EP(0x02, 2, 512) + EP(0x81, 2, 512))

DEV = struct.pack("<BBHBBBBHHHBBBB", 18, 1, 0x0200, 0, 0, 0, 64, 0x1234, 0x5678, 0x0100, 0, 0, 0, 1)
LINE_CODING = struct.pack("<IBBB", 115200, 0, 0, 8)
CBW = b"USBC" + struct.pack("<IIBBB", 0x1234, 36, 0x80, 0, 6) + bytes([0x12, 0, 0, 0, 36, 0]) + bytes(9)


def write(target, name, data):
    d = os.path.join(HERE, target, "testsuite")
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, name), "wb") as f:
        f.write(data)


for name, c in (("cdc_acm", CDC), ("msc", MSC), ("hid", HID)):
    write("fuzz_desc_check", name, bytes([3]) + c)
    extra = {"cdc_acm": LINE_CODING, "msc": b"\x00", "hid": REPORT}[name]
    write("fuzz_usbh_enumerate", name, bytes([2]) + DEV + struct.pack("<H", len(c)) + c + extra)

SETUP = lambda *a: b"\x00" + struct.pack("<BBHHH", *a)
CFG = SETUP(0x00, 9, 1, 0, 0)
write("fuzz_usbd_core", "get_device_desc", SETUP(0x80, 6, 0x0100, 0, 18))
write("fuzz_usbd_core", "get_config_desc", SETUP(0x80, 6, 0x0200, 0, 255))
write("fuzz_usbd_core", "cdc_line_coding", CFG + SETUP(0x21, 0x20, 0, 0, 7) + b"\x01\x00\x07\x00" + LINE_CODING)
write("fuzz_usbd_core", "msc_inquiry", CFG + SETUP(0xA1, 0xFE, 0, 2, 1) + b"\x01\x05" + struct.pack("<H", len(CBW)) + CBW)
write("fuzz_usbd_core", "hid_report_desc", CFG + SETUP(0x81, 6, 0x2200, 3, 74) + SETUP(0xA1, 1, 0x0100, 3, 4))
