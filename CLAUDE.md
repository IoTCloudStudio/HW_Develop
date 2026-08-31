# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

`HW_Develop` (IoT Cloud Studio) is a **hardware-development monorepo**, not a single buildable
project. It collects, per device: KiCad PCB designs, ESP8266/ESP32 firmware, 3D-printed
enclosure CAD + gcode, and technical documentation. Working language is Spanish — commit
messages, docs, and code comments are in Spanish; match that.

There is **no repo-wide build, test, or lint**. Each firmware project builds independently.
`cd` into the specific project directory before building anything.

### Top-level layout

| Path | Contents |
|---|---|
| `Productivo/` | Released products. Each has some of `PCB/` (KiCad), `Firmware/`, `Case/` (STL/gcode), `Documentacion*/` |
| `Testing/` | Prototypes, experiments, and **vendored** third-party repos/libraries (esp-rfid, esp-link, DSC Keybus interface, RDM6300, etc.) — do not treat these as our code |
| `Documentacion_Tecnica/` | Cross-cutting specs: MQTT protocol + serial-number nomenclature |
| `Kicad Modelos/` | Shared KiCad library of custom parts (legacy `.lib/.dcm/.mod` **and** modern `.kicad_sym/.kicad_mod`) |

## Firmware: two build systems

**PlatformIO projects** (have a `platformio.ini`) — `pio` is installed:
- `Productivo/Tesseract/Firmware V1/` and `Firmware V2/` (the flagship, see below)
- A few under `Testing/` (`Testing/Platformio/`, `Testing/Platformoio/`, `Testing/Control de Acceso/Repo original/`)

```bash
cd "Productivo/Tesseract/Firmware V2"
pio run -e generic          # build production firmware
pio run -e debug            # build with -DDEBUG (verbose serial)
pio run -e generic -t upload
pio run -t clean
pio device monitor -b 115200
```

**Arduino IDE sketches** (a folder containing `<name>.ino`, no `platformio.ini`) — most of
`Testing/`, plus `Productivo/RFW26/Firmware/`, `Productivo/Sensor_Puerta*/`. Build with the
Arduino IDE or `arduino-cli` (not installed here). Targets are ESP8266 or ESP32; several
sketches have Wi-Fi SSID / MQTT broker credentials hardcoded near the top of the `.ino`.

## Tesseract firmware (`Productivo/Tesseract/Firmware V2/`)

The main active codebase. It is a **fork of `esprfid/esp-rfid`** (Wiegand / MFRC522 / PN532 /
RDM6300 access control on ESP8266). Key things that are not obvious from the file tree:

- **Target**: `esp12e` (ESP8266), `espressif8266@2.3.2`, Arduino framework, 4M/2M flash
  (`-Wl,-Teagle.flash.4m2m.ld`). Envs: `generic`, `debug`, `relayboard` (`-DOFFICIALBOARD`).
- **Unity build**: `src/main.cpp` `#include`s the `src/*.esp` files directly (`led.esp`,
  `mqtt.esp`, `rfid.esp`, `wifi.esp`, `config.esp`, `websocket.esp`, `webserver.esp`, ...).
  `.esp` files are C++ (see `.gitattributes`). To add a module, create it and add an
  `#include "x.esp"` line in `main.cpp` — there is no per-file compilation.
- **Post-build**: `scripts/GENdeploy.py` / `DBGdeploy.py` / `OBdeploy.py` copy the built
  `firmware.bin` to `bin/generic.bin` / `bin/debug.bin` / `bin/forV2Board.bin`.
- **Web UI**: editable source in `src/websrc/`. The build tool `tools/webfilesbuilder/`
  (gulp) minifies + gzips + converts to PROGMEM byte-array headers in `src/webh/`, which the
  firmware serves. If you change the UI you must regenerate `src/webh/` **before** compiling:
  `cd tools/webfilesbuilder && npm install && npm start`.
  `tools/wsemulator/` runs the web UI against a fake WebSocket backend for hardware-free testing.
- **Config**: one large `struct Config` in `src/config.h`, persisted to SPIFFS as JSON,
  edited over WebSocket from the web UI. Reader-type IDs and other constants are in
  `src/magicnumbers.h`.
- **Crash debugging**: `DEBUGGING.md` — decode ESP8266 stack traces with
  EspArduinoExceptionDecoder against `.pio/build/debug/firmware.elf`.
- **CI**: `.github/workflows/platformio-build.yml` (scoped to this subfolder, not repo root)
  runs `pio run -e generic -e debug` on push.

## Cross-device conventions (`Documentacion_Tecnica/`)

- **MQTT "Cloud Connect" protocol** (`Protocolo_mqtt_cloudconnect_v1_0.md`) — the intended
  standard for new devices. Topic: `[TipoTopico]/[Modelo]/[NroSerie]/[TipoMQTT]`
  (`0`=Testing/`1`=Productivo, 2-byte model, 4-byte serial, `0`=data/`1`=command), e.g.
  `1/01A2/12345678/0`. JSON payload uses short keys: `L` log id, `T` epoch, `C` op code,
  `D` optional data, `V` protocol version.
  Note: the Tesseract/esp-rfid firmware predates this and uses its own `TOPIC/send` +
  `TOPIC/cmd` scheme documented in `Firmware V2/README-MQTT.md`.
- **Serial numbers** (`Nomenclador_NS_v1_0.md`): `[Fabricante]-[Lote]-[Modelo]-[IDPiezas]`,
  e.g. `1-0123-9-010002002054`.

## Other production devices

- **RFW26** (`Productivo/RFW26/`): 433 MHz RF → Wiegand-26 bridge. `RF_a_wiegand.ino`
  (ESP8266 + RCSwitch) receives a keyfob code and bit-bangs a Wiegand-26 frame with parity
  out D0/D1, feeding an access controller. PCB has `1 CAPA TESTING` (single-layer prototype)
  and `2 CAPAS PROD` (2-layer production) variants.
- **Lector RFID FDX Caravana** (`Productivo/Lector_RFID_FDX_Caravana/`): ESP32 reader for
  FDX-B animal ear-tags (ISO 11784/11785, 134.2 kHz). **The files under `Productivo/` were
  committed empty** (commit `c54ae28`); the real working prototype with source and docs is at
  `Testing/Caravana RFID/Lector_RFID_FDX_Caravana/`.
- **Cloud UPS**, **Comunicador Connect** (3G/4G gateway with OpenWRT configs under
  `Configuracion OpenWRT/`), **Sensor Puerta** — PCB + case + docs, older firmware.

## KiCad notes

- KiCad 6+ project files (`.kicad_pro/.kicad_sch/.kicad_pcb`). Custom parts live in
  `Kicad Modelos/` at repo root.
- `.dsn` / `.rules` / `.ses` files are Freerouting auto-router exchange files.
- PCB folders contain timestamped `*-backups/*.zip` — these are KiCad auto-backups, ignore them.
- `.gitignore` excludes `.pio/`, `.vscode/`, and `bin/*.bin`, but some subfolders have these
  committed anyway from before the rule.
