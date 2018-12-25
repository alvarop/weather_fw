#!/usr/bin/env python
"""
Decode incoming packets from weather station
"""

import argparse
import os
import struct
import serial
import sys
import sqlite3
import time
from crc import crc16
from datetime import datetime

HEADER_LEN = 4
CRC_LEN = 2
START_BYTES = 0xaa55

wx_keys = {
    "te": "temperature",
    "ws": "wind_speed",
    "wd": "wind_dir",
    "ra": "rain",
    "hu": "humidity",
    "li": "light",
    "pr": "pressure",
}

data_columns = [
    "timestamp",
    "temperature",
    "humidity",
    "pressure",
    "light",
    "rain",
    "wind_speed",
    "wind_dir",
]

sql_insert = "INSERT INTO samples VALUES(NULL,{})".format(
    ",".join(["?"] * len(data_columns))
)


def save_sqlite_data(data):
    line = []
    for key in data_columns:
        if key == "timestamp":
            line.append(data[key])
        else:
            line.append(int(data[key] * 1000))
    cur.execute(sql_insert, line)

    # Add retries in case the database is locked
    # Rpi zero is slow and the ostur_viewer hogs it when a request comes in
    retries = 5
    while retries > 0:
        try:
            con.commit()
            break
        except sqlite3.OperationalError:
            print("Unable to commit. Retrying {}".format(retries))
            retries -= 1
            continue


def save_csv_data(data):
    line = ""

    for key in data_columns:
        if key == "timestamp":
            line += datetime.fromtimestamp(data["timestamp"]).strftime(
                "%Y-%m-%d %H:%M:%S"
            )
        else:
            line += str(data[key])

        line += ","

    csvfile.write(line + "\n")
    csvfile.flush()


def write_csv_header():
    header = ""
    for item in data_columns:
        header += str(item) + ","
    csvfile.write(header + "\n")


def print_data(data):
    line = ""

    for key in data_columns:
        if key == "timestamp":
            line += datetime.fromtimestamp(data["timestamp"]).strftime(
                "%Y-%m-%d %H:%M:%S"
            )
        else:
            line += str(data[key])
        line += ","

    print(line)


def check_crc(packet):
    packet_crc = struct.unpack("H", packet[-CRC_LEN:])[0]
    computed_crc = crc16(packet[0:-CRC_LEN])
    return packet_crc == computed_crc


def process_wx_str(wx_str):
    data = {}
    data["timestamp"] = int(time.time())

    for line in wx_str.split("\n"):
        key, val = line.split(":")
        if key in wx_keys:
            data[wx_keys[key]] = float(val)
        else:
            data[key] = val

    if args.csvfile:
        save_csv_data(data)

    if args.db:
        save_sqlite_data(data)

    print_data(data)


def process_packet(packet):
    try:
        packet_str = packet[HEADER_LEN:-CRC_LEN].decode("utf-8").strip()
        process_wx_str(packet_str)

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


parser = argparse.ArgumentParser()

parser.add_argument("--baud_rate", default=115200, type=int, help="xbee baud rate")

parser.add_argument("--port", required=True, help="xbee device to connect to")

parser.add_argument("--csvfile", help="Output csvfile (csv format)")

parser.add_argument("--db", help="Sqlite db csvfile")

args = parser.parse_args()

if args.csvfile and os.path.isfile(args.csvfile):
    need_header = False
else:
    need_header = True

if args.csvfile:
    csvfile = open(args.csvfile, mode="a")

    if need_header:
        write_csv_header()

if args.db:
    con = None
    con = sqlite3.connect(args.db)

    if con is None:
        raise IOError("Unable to open sqlite database")

    cur = con.cursor()
    cur.execute(
        "CREATE TABLE IF NOT EXISTS "
        + "samples(id INTEGER PRIMARY KEY,{} INTEGER)".format(
            " INTEGER, ".join(data_columns)
        )
    )

stream = serial.Serial(args.port, baudrate=args.baud_rate, timeout=0.01)
stream.flushInput()

buff = bytearray()

while True:
    line = stream.read(1)
    if len(line) > 0:
        buff.append(line[0])
        while decode(buff) is True:
            pass
