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
import collections

HEADER_LEN = 4
CRC_LEN = 2
START_BYTES = 0xaa55

# timestamp and uid must be the first two
data_columns = [
    "timestamp",
    "uid",
    "temperature",
    "humidity",
    "pressure",
    "temperatre_in",
    "light",
    "battery",
    "rain",
    "wind_speed",
    "wind_dir",
]

WeatherData = collections.namedtuple(
    "WeatherData",
    [
        "uid",
        "packet_type",
        "wind_speed",
        "wind_dir",
        "rain",
        "temperature",
        "humidity",
        "temperatre_in",
        "pressure",
        "light",
        "battery",
    ],
)

GPSData = collections.namedtuple(
    "GPSData",
    [
        "uid",
        "packet_type",
        "lat_degrees",
        "lat_minutes",
        "lat_cardinal",
        "lon_degrees",
        "lon_minutes",
        "lon_cardinal",
    ],
)


PACKET_TYPE_DATA = 1
PACKET_TYPE_GPS = 2

sql_insert = "INSERT INTO samples VALUES(NULL,{})".format(
    ",".join(["?"] * len(data_columns))
)

devices = {}


def process_data_packet(packet):
    data = WeatherData._make(struct.unpack_from("<IBfffffffff", packet, 0))
    print(data)

    if args.db:
        save_sqlite_data(data)

    if args.csvfile:
        save_csv_data(data)


def process_gps_packet(packet):
    data = GPSData._make(struct.unpack_from("<IBidcidc", packet, 0))
    print(data)

    if args.db:
        # TODO - only add this if not set
        add_sqlite_device(data.uid, None, get_gps_string(data))


packet_processors = {
    PACKET_TYPE_DATA: process_data_packet,
    PACKET_TYPE_GPS: process_gps_packet,
}


def get_gps_string(gps_data):
    gps_str = "{} {} {} {} {} {}".format(
        gps_data.lat_degrees,
        gps_data.lat_minutes,
        gps_data.lat_cardinal,
        gps_data.lon_degrees,
        gps_data.lon_minutes,
        gps_data.lon_cardinal,
    )

    return gps_str


def read_sqlite_devices():
    query = "select * from devices"
    cur.execute(query)
    rows = cur.fetchall()

    for row in rows:
        devices[row[0]] = row[1]


def add_sqlite_device(uid, name, gps_str):

    if name is None:
        name = devices[uid]

    sql_query = 'REPLACE INTO devices (uid, name, gps) VALUES ({}, "{}", "{}");'.format(
        uid, name, gps_str
    )

    cur.execute(sql_query)

    # Add retries in case the database is locked
    retries = 5
    while retries > 0:
        try:
            con.commit()
            break
        except sqlite3.OperationalError:
            print("Unable to commit. Retrying {}".format(retries))
            retries -= 1
            continue

    devices[uid] = name


def save_sqlite_data(data):

    if data.uid not in devices:
        print("New device! {}".format(data.uid))
        add_sqlite_device(data.uid, str(data.uid), "")

    line = [int(time.time())]

    for key in data_columns:
        if key != "timestamp":
            line.append(getattr(data, key))

    cur.execute(sql_insert, line)

    # Add retries in case the database is locked
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
    line = datetime.fromtimestamp(time.time()).strftime("%Y-%m-%d %H:%M:%S")

    for key in data_columns:
        if key != "timestamp":
            line += str(getattr(data, key))

        line += ","

    csvfile.write(line + "\n")
    csvfile.flush()


def write_csv_header():
    header = ""
    for item in data_columns:
        header += str(item) + ","
    csvfile.write(header + "\n")


def check_crc(packet):
    packet_crc = struct.unpack("H", packet[-CRC_LEN:])[0]
    computed_crc = crc16(packet[0:-CRC_LEN])
    return packet_crc == computed_crc


def process_packet(packet):
    print(packet)
    try:
        packet_bytes = packet[HEADER_LEN:-CRC_LEN]
        uid, packet_type = struct.unpack_from("IB", packet_bytes, 0)

        if packet_type in packet_processors:
            packet_processors[packet_type](packet_bytes)
        else:
            print("ERR: Unknown type", packet_type)

    except UnicodeDecodeError:
        print("unicode error")
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
        + "samples(id INTEGER PRIMARY KEY, timestamp INTEGER, uid INTEGER, "
        + "{} FLOAT)".format(" FLOAT, ".join(data_columns[2:]))
    )

    cur.execute(
        "CREATE TABLE IF NOT EXISTS "
        + "devices(uid INTEGER PRIMARY KEY, name TEXT, gps TEXT)"
    )

    read_sqlite_devices()

stream = serial.Serial(args.port, baudrate=args.baud_rate, timeout=0.01)
stream.flushInput()

buff = bytearray()

while True:
    line = stream.read(1)
    if len(line) > 0:
        buff.append(line[0])
        while decode(buff) is True:
            pass
