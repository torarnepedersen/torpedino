#!/usr/bin/env python3
"""
Genererer torpedino_png.ino og pinwizard_html.ino fra kildefilene.
Kjøres fra rooten av prosjektet:

    python3 tools/make_assets.py   (Linux/Mac)
    python  tools/make_assets.py   (Windows)

Avhengigheter: Pillow  (pip install pillow)
"""

import gzip, io, os, sys
from PIL import Image

base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def make_ino(varname, data, note):
    lines = []
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk))
    hex_body = ",\n".join(lines)
    return (
        f"// Auto-generert – ikke endre manuelt.\n"
        f"// Regenerer: python3 tools/make_assets.py  ({note})\n\n"
        f"const uint8_t {varname}[]     PROGMEM = {{\n{hex_body}\n}};\n"
        f"const size_t  {varname}_len            = {len(data)};\n"
    )

# ── torpedino.png → skalert til halvparten → torpedino_png.ino ──────────────
png_path = os.path.join(base, "torpedino.png")
if not os.path.exists(png_path):
    print(f"Feil: {png_path} ikke funnet", file=sys.stderr)
    sys.exit(1)

img = Image.open(png_path)
img_half = img.resize((img.width // 2, img.height // 2), Image.LANCZOS)
buf = io.BytesIO()
img_half.save(buf, "PNG", optimize=True)
png_data = buf.getvalue()

dest = os.path.join(base, "torpedino_png.ino")
with open(dest, "w") as f:
    f.write(make_ino("torpedino_png", png_data,
                     f"torpedino.png skalert til {img_half.size[0]}×{img_half.size[1]}"))
print(f"torpedino_png.ino  oppdatert – {len(png_data):,} bytes  "
      f"({img.size[0]}×{img.size[1]} → {img_half.size[0]}×{img_half.size[1]})")

# ── pinwizard.html → gzip-komprimert → pinwizard_html.ino ───────────────────
html_path = os.path.join(base, "pinwizard.html")
if not os.path.exists(html_path):
    print(f"Feil: {html_path} ikke funnet", file=sys.stderr)
    sys.exit(1)

with open(html_path, "rb") as f:
    html_raw = f.read()
html_gz = gzip.compress(html_raw, compresslevel=9)

dest = os.path.join(base, "pinwizard_html.ino")
with open(dest, "w") as f:
    f.write(make_ino("pinwizard_html", html_gz,
                     f"pinwizard.html gzip {len(html_raw):,}→{len(html_gz):,} bytes"))
print(f"pinwizard_html.ino oppdatert – {len(html_raw):,} → {len(html_gz):,} bytes (gzip)")
