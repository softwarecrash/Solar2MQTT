#!/usr/bin/env python3
from pathlib import Path
import csv

ROOT = Path(__file__).resolve().parents[1]
CSV_PATH = ROOT / "partitions" / "atom_lite_littlefs.csv"
APP = ROOT / ".pio" / "build" / "m5stack_atom_lite" / "firmware.bin"
FLASH_SIZE = 0x400000
MIN_APP_HEADROOM = 128 * 1024
MIN_LITTLEFS = 384 * 1024

def parse_num(value: str) -> int:
    value = value.strip()
    return int(value, 0)

rows = []
with CSV_PATH.open("r", encoding="utf-8") as fh:
    for raw in fh:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        cols = [x.strip() for x in next(csv.reader([raw]))]
        if len(cols) < 5:
            raise SystemExit(f"Malformed partition row: {raw.rstrip()}")
        name, ptype, subtype, offset, size = cols[:5]
        rows.append({
            "name": name,
            "type": ptype,
            "subtype": subtype,
            "offset": parse_num(offset),
            "size": parse_num(size),
        })

if not rows:
    raise SystemExit("Partition table is empty")

by_name = {row["name"]: row for row in rows}
required = {"nvs", "otadata", "app0", "app1", "backlog", "coredump"}
missing = required - by_name.keys()
if missing:
    raise SystemExit(f"Missing partitions: {sorted(missing)}")

# Preserve the legacy settings area so USB migration can keep existing NVS.
if by_name["nvs"]["offset"] != 0x9000 or by_name["nvs"]["size"] != 0x5000:
    raise SystemExit("NVS must remain at 0x9000 size 0x5000")
if by_name["otadata"]["offset"] != 0xE000 or by_name["otadata"]["size"] != 0x2000:
    raise SystemExit("OTA data must remain at 0xE000 size 0x2000")

ordered = sorted(rows, key=lambda r: r["offset"])
for prev, cur in zip(ordered, ordered[1:]):
    prev_end = prev["offset"] + prev["size"]
    if prev_end > cur["offset"]:
        raise SystemExit(
            f"Partition overlap: {prev['name']} ends 0x{prev_end:X}, "
            f"{cur['name']} starts 0x{cur['offset']:X}"
        )

last_end = max(r["offset"] + r["size"] for r in rows)
if last_end > FLASH_SIZE:
    raise SystemExit(f"Partition table exceeds 4MB flash: end=0x{last_end:X}")

if by_name["backlog"]["type"] != "data" or by_name["backlog"]["subtype"].lower() != "littlefs":
    raise SystemExit("Backlog partition must be data/littlefs")
if by_name["backlog"]["size"] < MIN_LITTLEFS:
    raise SystemExit("LittleFS backlog is smaller than 384 KiB")

if by_name["app0"]["size"] != by_name["app1"]["size"]:
    raise SystemExit("OTA app partitions must have equal sizes")

if not APP.exists():
    raise SystemExit(f"Firmware binary not found: {APP}")
app_size = APP.stat().st_size
app_capacity = by_name["app0"]["size"]
headroom = app_capacity - app_size
if headroom < MIN_APP_HEADROOM:
    raise SystemExit(
        f"Insufficient OTA headroom: app={app_size}, capacity={app_capacity}, "
        f"headroom={headroom}, required={MIN_APP_HEADROOM}"
    )

print("ATOM Lite partition validation OK")
print(f"  app0/app1: {app_capacity} bytes each")
print(f"  firmware:  {app_size} bytes")
print(f"  headroom:  {headroom} bytes ({headroom / app_capacity * 100:.1f}%)")
print(f"  LittleFS:  {by_name['backlog']['size']} bytes")
print(f"  flash end: 0x{last_end:X}")
