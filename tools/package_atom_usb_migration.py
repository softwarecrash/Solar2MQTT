#!/usr/bin/env python3
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / ".pio" / "build" / "m5stack_atom_lite"
OUT = ROOT / ".firmware" / "usb-migration-15"
PIO_HOME = Path.home() / ".platformio"
BOOT_APP0 = PIO_HOME / "packages" / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin"

OUT.mkdir(parents=True, exist_ok=True)

required = {
    "bootloader.bin": BUILD / "bootloader.bin",
    "partitions.bin": BUILD / "partitions.bin",
    "firmware.bin": BUILD / "firmware.bin",
    "boot_app0.bin": BOOT_APP0,
    "partition-table.csv": ROOT / "partitions" / "atom_lite_littlefs.csv",
}
for name, src in required.items():
    if not src.exists():
        raise SystemExit(f"Missing required migration file: {src}")
    shutil.copy2(src, OUT / name)

readme = r"""Solar2MQTT ATOM Lite USB migration 0.0.0-dev-2.0.17
==============================================================

Purpose
-------
One-time migration from the old no-filesystem partition layout to:
  NVS      0x009000..0x00DFFF   (preserved settings)
  OTA data 0x00E000..0x00FFFF
  app0     0x010000..0x1CFFFF
  app1     0x1D0000..0x38FFFF
  LittleFS 0x390000..0x3EFFFF   (384 KiB MQTT backlog)
  coredump 0x3F0000..0x3FFFFF

IMPORTANT
---------
Do NOT erase the flash before this migration if you want to keep Wi-Fi,
MQTT and device settings stored in NVS.

The migration writes only bootloader, partition table, OTA metadata and app0.
It deliberately does NOT write or erase 0x9000..0xDFFF (NVS).

Windows / PlatformIO
--------------------
Run:
    flash_migration_pio.bat COM5

Replace COM5 with the ATOM Lite serial port.

Windows / Python esptool
------------------------
Run:
    flash_migration_python.bat COM5

This requires the Python "esptool" package.

After a successful migration
----------------------------
1. Let the module boot.
2. Existing NVS settings should still be present.
3. LittleFS is formatted automatically on first mount.
4. Future firmware updates use the normal .bin.ota file / OTA updater.
"""

pio_bat = r"""@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 COM_PORT
  echo Example: %~nx0 COM5
  exit /b 2
)
set PORT=%~1
set PIOPY=%USERPROFILE%\.platformio\penv\Scripts\python.exe
if not exist "%PIOPY%" (
  echo PlatformIO Python not found: %PIOPY%
  exit /b 3
)
echo Flashing migration to %PORT% at 115200 WITHOUT erasing NVS...
"%PIOPY%" -m esptool --chip esp32 --port %PORT% --baud 115200 write-flash -z ^
  0x1000 bootloader.bin ^
  0x8000 partitions.bin ^
  0xe000 boot_app0.bin ^
  0x10000 firmware.bin
if errorlevel 1 (
  echo.
  echo Flash failed. Make sure esptool is installed in PlatformIO Python:
  echo   "%PIOPY%" -m pip install esptool
  exit /b %errorlevel%
)
echo.
echo Migration completed. Do not run erase_flash.
endlocal
"""

python_bat = r"""@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 COM_PORT
  echo Example: %~nx0 COM5
  exit /b 2
)
set PORT=%~1
echo Flashing migration to %PORT% WITHOUT erasing NVS...
python -m esptool --chip esp32 --port %PORT% --baud 115200 write-flash -z ^
  0x1000 bootloader.bin ^
  0x8000 partitions.bin ^
  0xe000 boot_app0.bin ^
  0x10000 firmware.bin
if errorlevel 1 exit /b %errorlevel%
echo.
echo Migration completed. Do not run erase_flash.
endlocal
"""

(OUT / "README.txt").write_text(readme, encoding="utf-8", newline="\n")
(OUT / "flash_migration_pio.bat").write_text(pio_bat, encoding="utf-8", newline="\r\n")
(OUT / "flash_migration_python.bat").write_text(python_bat, encoding="utf-8", newline="\r\n")

print(f"USB migration package created: {OUT}")
for path in sorted(OUT.iterdir()):
    print(f"  {path.name}: {path.stat().st_size} bytes")

zip_base = ROOT / ".firmware" / "Solar2MQTT_m5stack_atom_lite_USB_MIGRATION_V0.0.0-dev-2.0.17"
archive = shutil.make_archive(str(zip_base), "zip", root_dir=OUT)
print(f"USB migration ZIP: {archive}")
