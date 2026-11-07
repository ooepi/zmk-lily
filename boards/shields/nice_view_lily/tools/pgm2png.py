"""Convert preview.pgm (written by preview.c) to preview.png using only the standard library."""

import struct
import sys
import zlib
from pathlib import Path

src = Path(sys.argv[1] if len(sys.argv) > 1 else "preview.pgm")
data = src.read_bytes()
header, _, pixels = data.partition(b"255\n")
w, h = map(int, header.split()[1:3])
raw = b"".join(b"\0" + pixels[y * w : (y + 1) * w] for y in range(h))


def chunk(tag, body):
    return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body))


png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")
src.with_suffix(".png").write_bytes(png)
print(f"wrote {src.with_suffix('.png')} ({w}x{h})")
