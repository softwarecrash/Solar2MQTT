Import("env")

import os
import shutil

ENABLE_MERGE_BIN = True

BUILD_DIR = env.subst("$BUILD_DIR")
PROGNAME = env.subst("${PROGNAME}")
APP_BIN = os.path.normpath(os.path.join(BUILD_DIR, f"{PROGNAME}.bin"))
MERGED_BIN = os.path.normpath(os.path.join(BUILD_DIR, f"{PROGNAME}_merged.bin"))
BOARD_CONFIG = env.BoardConfig()

project_name = env.GetProjectOption("custom_source_name") or "Firmware"
version = env.GetProjectOption("custom_source_version") or "0.0.0"
env_name = env.subst("$PIOENV") or "unknown"

firmware_filename_merged = f"{project_name}_{env_name}_V{version}.bin"
firmware_filename_ota = f"{project_name}_{env_name}_V{version}.bin.ota"

firmware_path_merged = os.path.normpath(
    os.path.join(env.subst("$PROJECT_DIR"), ".firmware", firmware_filename_merged)
)
firmware_path_ota = os.path.normpath(
    os.path.join(env.subst("$PROJECT_DIR"), ".firmware", firmware_filename_ota)
)


def copy_bin_as_ota(source, target, env):
    os.makedirs(os.path.dirname(firmware_path_ota), exist_ok=True)
    shutil.copyfile(APP_BIN, firmware_path_ota)
    print(f"Firmware (OTA) copied to: {firmware_path_ota}")


def merge_bin(source, target, env):
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", []))
    app_offset = env.subst("$ESP32_APP_OFFSET") or "0x10000"
    flash_images += [app_offset, APP_BIN]

    flash_args = [
        os.path.normpath(env.subst(str(item))) if not str(item).startswith("0x") else str(item)
        for item in flash_images
    ]

    cmd = [
        env.subst("$PYTHONEXE"),
        env.subst("$OBJCOPY"),
        "--chip",
        BOARD_CONFIG.get("build.mcu", "esp32"),
        "merge-bin",
        "--pad-to-size",
        BOARD_CONFIG.get("upload.flash_size", "4MB"),
        "--output",
        MERGED_BIN,
    ] + flash_args

    command = " ".join(f'"{item}"' if " " in item else item for item in cmd)
    result = env.Execute(command)

    os.makedirs(os.path.dirname(firmware_path_merged), exist_ok=True)
    shutil.copyfile(MERGED_BIN, firmware_path_merged)
    print(f"Firmware (merged) copied to: {firmware_path_merged}")
    return result


env.AddPostAction(APP_BIN, copy_bin_as_ota)
if ENABLE_MERGE_BIN:
    env.AddPostAction(APP_BIN, merge_bin)
