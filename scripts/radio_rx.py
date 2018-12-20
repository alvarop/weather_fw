#!/usr/bin/env python
'''
Decode incoming packets from weather station
'''

import subprocess
import time
import struct
import serial
import sys
from crc import crc16

HEADER_LEN = 4
CRC_LEN = 2
START_BYTES = 0xaa55

def check_crc(packet):
    packet_crc = struct.unpack("H", packet[-CRC_LEN:])[0]
    computed_crc = crc16(packet[0:-CRC_LEN])
    return packet_crc == computed_crc


def process_packet(packet):
    try:
        print(packet[HEADER_LEN:-CRC_LEN].decode('utf-8').strip())
    except UnicodeDecodeError:
        pass


def decode(buff):

    # Look for start bytes in packet
    for offset in range(len(buff)):

        # Need at least 4 bytes for the header and 2 for crc
        if (len(buff) - offset) < (HEADER_LEN + CRC_LEN):
            return False

        start, dlen = struct.unpack_from("HH", buff, offset)
        if start == START_BYTES:

            # Make sure we have enough bytes for the packet
            if (len(buff) - offset) < (HEADER_LEN + dlen + CRC_LEN):
                return False
            else:
                if (
                    check_crc(buff[offset : (offset + dlen + HEADER_LEN + CRC_LEN)])
                    is True
                ):
                    process_packet(
                        buff[offset : (offset + dlen + HEADER_LEN + CRC_LEN)]
                    )

                    # Remove all data before the packet
                    del buff[: (offset + dlen + HEADER_LEN + CRC_LEN)]
                    return True
                else:
                    # CRC Error, remove the header and keep processing
                    del buff[: (offset + 2)]
                    return True


stream = serial.Serial(sys.argv[1], baudrate=115200)
stream.timeout = 0.01

buff = bytearray()

while True:
    line = stream.read(1)
    if len(line) > 0:
        buff.append(line[0])
        while decode(buff) is True:
            pass

