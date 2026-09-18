# Torpedino firmware

Firmware for Torpedino – et egendefinert undervisningsbrett basert på ESP32.

## Brettvarianter

| Mappe | Mikrokontroller | Status |
|---|---|---|
| `esp32s2/` | ESP32-S2-WROOM | Aktiv |
| `esp32c5/` | ESP32-C5 | Under utvikling |

## Kom i gang

1. Åpne Arduino-sketchen for ønsket variant, f.eks. `esp32s2/torpedino/torpedino.ino`
2. Flash via USB første gang, deretter via OTA

De auto-genererte filene (`torpedino_png.ino`, `pinwizard_html.ino`) følger med i repoet, så dette steget holder for å bare flashe. Hvis du endrer `pinwizard.html` eller `torpedino.png`, må du regenerere dem selv (krever Pillow: `pip install pillow`):
```
python3 tools/make_assets.py
```

## Ny release

1. Kjør `tools/make_assets.py` (krever Pillow: `pip install pillow`)
2. Kjør `tools/make_release_zip.py`
3. Last opp `torpedino-s2.zip` til en ny GitHub Release tagget f.eks. `s2-v0.0003`
4. Oppdater `DOWNLOAD_URL` i `core.ino` til å peke på ny release
