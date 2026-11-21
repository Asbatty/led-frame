"""Save a picture of exactly what the LED frame is drawing right now.

    python tools/snapshot.py                 -> frame.png (from http://matrix.local)
    python tools/snapshot.py 192.168.1.50 out.png

Reads /api/frame (the firmware's off-screen canvas) and draws each pixel as an LED.
Needs only the Python standard library.
"""
import struct
import sys
import urllib.request
import zlib

host = sys.argv[1] if len(sys.argv) > 1 else "matrix.local"
out = sys.argv[2] if len(sys.argv) > 2 else "frame.png"
SCALE, W, H = 10, 64, 32

text = urllib.request.urlopen(f"http://{host}/api/frame", timeout=10).read().decode()
rows = [r for r in text.split("\n") if r]
pix = []
for r in rows[:H]:
    cells = [r[i:i + 4].strip() for i in range(0, len(r), 4)]
    line = []
    for c in cells[:W]:
        v = 0 if c in (".", "") else int(c, 16)
        line.append((((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31))
    pix.append(line)

raw = bytearray()
for y in range(H * SCALE):
    raw.append(0)
    for x in range(W * SCALE):
        cx, cy = x % SCALE - SCALE / 2 + 0.5, y % SCALE - SCALE / 2 + 0.5
        lit = cx * cx + cy * cy <= (SCALE * 0.42) ** 2
        r, g, b = pix[y // SCALE][x // SCALE] if lit else (8, 8, 8)
        if lit and (r, g, b) == (0, 0, 0):
            r = g = b = 24
        raw += bytes((r, g, b))


def chunk(tag, data):
    c = struct.pack(">I", len(data)) + tag + data
    return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)


png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W * SCALE, H * SCALE, 8, 2, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
open(out, "wb").write(png)
print(f"Saved {out}")
