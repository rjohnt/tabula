#!/usr/bin/env python3
"""Copy the Tabula card image onto the X3's SD card over USB serial.

The X3 (ESP32-C3) has no USB drive mode, so a tabula_dev build accepts
"CMD:PUT <size> <crc32> <path>" followed by the raw bytes, and "CMD:STAT <path>"
to skip files already there at the same size (rerun to resume). Keep the reader
awake (each command counts as activity once it is running).

  .venv/bin/python docs/tabula/tools/load_card.py --port /dev/cu.usbmodem2101 [--src build/tabula-sd]
"""

import argparse
import sys
import time
import zlib
from pathlib import Path

import serial

REPO = Path(__file__).resolve().parents[3]
CHUNK = 8192  # must match PUT_CHUNK in src/main.cpp


def wait_for(ser, prefixes, timeout=20.0):
    """Next line starting with one of `prefixes`; log lines in between are skipped."""
    end = time.time() + timeout
    buf = b""
    while time.time() < end:
        chunk = ser.readline()
        if not chunk:
            continue
        buf = chunk.decode(errors="replace").strip()
        for p in prefixes:
            if len(p) > 1:
                # Log lines from other tasks can run into a reply on the same line.
                at = buf.find(p)
                if at >= 0:
                    return buf[at:]
            elif buf == p or buf.startswith(p + "[") or buf.endswith(p):
                return p
    raise TimeoutError(f"no {prefixes} (last: {buf!r})")


def remote_size(ser, card):
    ser.write(f"CMD:STAT {card}\n".encode())
    return int(wait_for(ser, ["STAT "]).split()[1])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--src", type=Path, default=REPO / "build/tabula-sd")
    ap.add_argument("--only", default=None, help="substring filter on card paths")
    ap.add_argument("--from", dest="start", default=None, help="skip card paths that sort before this one")
    ap.add_argument("--force", action="store_true", help="resend even when the card has a file of the same size")
    a = ap.parse_args()

    files = sorted(p for p in a.src.rglob("*") if p.is_file() and p.name != ".DS_Store")
    if a.only:
        files = [p for p in files if a.only in str(p.relative_to(a.src))]
    if a.start:
        files = [p for p in files if "/" + p.relative_to(a.src).as_posix() >= a.start]
    total = sum(p.stat().st_size for p in files)
    ser = serial.Serial(a.port, 115200, timeout=0.5)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.3)
    ser.reset_input_buffer()

    sent = skipped = 0
    done_bytes = 0
    t0 = time.time()
    for i, p in enumerate(files, 1):
        card = "/" + p.relative_to(a.src).as_posix()
        data = p.read_bytes()
        if not a.force and remote_size(ser, card) == len(data):
            skipped += 1
        else:
            for attempt in range(3):
                ser.write(f"CMD:PUT {len(data)} {zlib.crc32(data):08x} {card}\n".encode())
                reply = wait_for(ser, ["PUT_READY", "PUT_ERR"])
                if reply.startswith("PUT_ERR"):
                    raise RuntimeError(f"{card}: {reply}")
                try:
                    for off in range(0, len(data), CHUNK):
                        ser.write(data[off:off + CHUNK])
                        if wait_for(ser, ["K", "PUT_ERR"]) != "K":
                            break
                    reply = wait_for(ser, ["PUT_OK", "PUT_ERR"], timeout=30)
                except TimeoutError as e:
                    # A lost reply: the card is the truth. Let the device finish
                    # or abandon the transfer, then look at what was written.
                    time.sleep(6)
                    ser.reset_input_buffer()
                    reply = "PUT_OK" if remote_size(ser, card) == len(data) else f"PUT_ERR lost ({e})"
                if reply.startswith("PUT_OK"):
                    break
                print(f"\n{card}: {reply}, retrying", file=sys.stderr)
            else:
                raise RuntimeError(f"{card}: failed 3 times")
            sent += 1
        done_bytes += len(data)
        rate = done_bytes / max(0.1, time.time() - t0) / 1e6
        print(f"\r{i}/{len(files)}  {done_bytes / 1e6:6.1f}/{total / 1e6:.1f} MB  {rate:4.2f} MB/s  "
              f"sent {sent} skipped {skipped}   ", end="", flush=True)
    print(f"\nDone in {time.time() - t0:.0f} s")


if __name__ == "__main__":
    main()
