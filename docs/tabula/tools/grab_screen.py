#!/usr/bin/env python3
"""Save what the X3 is showing right now as a 1-bit PNG (portrait 528x792).

Uses the firmware's CMD:SCREENSHOT, which sends the raw framebuffer over the
USB serial link. Opening the port resets the X3, so --serve keeps it open:
navigate on the device, then touch <dir>/<name>.req to save <dir>/<name>.png.

    .venv/bin/python docs/tabula/tools/grab_screen.py --serve shots/
"""
import argparse
import sys
import time
from pathlib import Path

import serial
from PIL import Image

PANEL_W, PANEL_H = 792, 528  # X3 panel, native landscape


def open_port(port):
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = port, 115200, 0.5
    ser.dtr = ser.rts = False
    ser.open()
    return ser


def grab(ser):
    if True:
        ser.reset_input_buffer()
        ser.write(b"CMD:SCREENSHOT\n")
        buf, deadline = b"", time.time() + 10
        while b"SCREENSHOT_START:" not in buf:
            if time.time() > deadline:
                sys.exit("no SCREENSHOT_START (is the device awake?)")
            buf += ser.read(256)
        head, rest = buf.split(b"SCREENSHOT_START:", 1)
        size_line, data = rest.split(b"\n", 1)
        size = int(size_line)
        while len(data) < size:
            if time.time() > deadline + 20:
                sys.exit(f"short framebuffer: {len(data)}/{size}")
            data += ser.read(size - len(data))
        return data[:size]


def main():
    p = argparse.ArgumentParser()
    p.add_argument("out", help="PNG path, or a directory with --serve")
    p.add_argument("--serve", action="store_true")
    p.add_argument("--port", default="/dev/cu.usbmodem2101")
    p.add_argument("--rotate", type=int, default=270, help="degrees counter-clockwise to portrait")
    a = p.parse_args()
    ser = open_port(a.port)
    if not a.serve:
        save(grab(ser), a.out, a.rotate)
        return
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    print(f"serving: touch {out}/<name>.req", flush=True)
    log = open(out / "device.log", "ab")
    last_ping = time.time()
    while True:
        try:
            for req in sorted(out.glob("*.req")):
                req.unlink()
                save(grab(ser), out / (req.stem + ".png"), a.rotate)
            if time.time() - last_ping > 30:
                # Serial commands count as activity on dev builds, so the X3 stays awake.
                ser.write(b"CMD:STAT /\n")
                last_ping = time.time()
            log.write(ser.read(ser.in_waiting or 1))
            log.flush()
        except (serial.SerialException, OSError) as e:
            # The X3 restarted (or slept); its USB port comes back with it.
            print(f"port lost ({e}); waiting", flush=True)
            ser.close()
            while not Path(a.port).exists():
                time.sleep(0.5)
            time.sleep(1)
            ser = open_port(a.port)
            print("reconnected", flush=True)


def save(data, path, rotate):
    if len(data) * 8 != PANEL_W * PANEL_H:
        sys.exit(f"unexpected framebuffer size {len(data)}")
    Image.frombytes("1", (PANEL_W, PANEL_H), data).rotate(rotate, expand=True).save(path)
    print(path, flush=True)


if __name__ == "__main__":
    main()
