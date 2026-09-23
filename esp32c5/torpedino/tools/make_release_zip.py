#!/usr/bin/env python3
"""
Lager en release-zip klar for opplasting til GitHub Releases.

Kjøres fra esp32c5/torpedino/:
    python3 tools/make_release_zip.py

Forutsetning: make_assets.py må ha vært kjørt først slik at
torpedino_png.ino og pinwizard_html.ino eksisterer.

Output: torpedino-c5.zip i samme mappe (overskrives hvis den finnes).
"""

import zipfile, os, sys

base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

files = [
    "torpedino.ino",
    "core.ino",
    "torpedino_png.ino",
    "pinwizard_html.ino",
    "tools/make_assets.py",
    "tools/make_release_zip.py",
]

missing = [f for f in files if not os.path.exists(os.path.join(base, f))]
if missing:
    print("Mangler filer (kjør make_assets.py først?):", missing, file=sys.stderr)
    sys.exit(1)

out_path = os.path.join(base, "torpedino-c5.zip")
with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as zf:
    for f in files:
        zf.write(os.path.join(base, f), f"torpedino/{f}")

size = os.path.getsize(out_path)
print(f"torpedino-c5.zip  –  {size:,} bytes  ({len(files)} filer)")
print(f"Last opp til GitHub Releases: {out_path}")
