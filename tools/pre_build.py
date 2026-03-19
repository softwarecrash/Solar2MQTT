import gzip
import glob
import os

WWW_DIR = os.path.join("src", "webUI")
OUTPUT_HEADER_FILE = os.path.join("src", "www.h")
SUPPORTED_EXTENSIONS = ["*.html", "*.js", "*.css", "*.svg", "*.ico", "*.png"]

MIME_TYPES = {
    ".css": "text/css",
    ".html": "text/html",
    ".ico": "image/x-icon",
    ".js": "text/javascript",
    ".png": "image/png",
    ".svg": "image/svg+xml",
}


def guess_mime_type(filename):
    _, ext = os.path.splitext(filename.lower())
    return MIME_TYPES.get(ext, "application/octet-stream")


def emit_asset(path):
    with open(path, "rb") as handle:
        compressed = gzip.compress(handle.read(), compresslevel=9)

    array_name = os.path.relpath(path, WWW_DIR).replace(os.sep, "_").replace(".", "_")
    lines = [f"const uint8_t {array_name}_gz[] PROGMEM = {{\n"]

    for offset in range(0, len(compressed), 16):
        chunk = ", ".join(f"0x{byte:02x}" for byte in compressed[offset : offset + 16])
        lines.append(f"  {chunk},\n")

    lines.append("};\n\n")
    lines.append(f"const unsigned int {array_name}_gz_len = {len(compressed)};\n")
    lines.append(f"const char *{array_name}_gz_mime = \"{guess_mime_type(path)}\";\n\n")
    print(f"Embedded: {os.path.relpath(path, WWW_DIR)}")
    return "".join(lines)


def main():
    if not os.path.isdir(WWW_DIR):
        print(f"Skip asset build, missing: {WWW_DIR}")
        return

    files = set()
    for pattern in SUPPORTED_EXTENSIONS:
        files.update(glob.iglob(os.path.join(WWW_DIR, "**", pattern), recursive=True))

    with open(OUTPUT_HEADER_FILE, "w", encoding="utf-8") as handle:
        handle.write("#ifndef WWW_H\n#define WWW_H\n\n#include <pgmspace.h>\n\n")
        for path in sorted(files):
            handle.write(emit_asset(path))
        handle.write("#endif\n")


main()
