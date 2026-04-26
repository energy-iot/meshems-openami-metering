# Changelog

## [Unreleased] - 2026-04-25

### Fixed

#### FastLED integration (`platformio.ini`)
- Added `fastled/FastLED @ 3.7.8` to `lib_deps` — the library was commented out, causing a missing `FastLED.h` compile error
- Pinned FastLED to **3.7.8** to avoid a `operator new` redefinition conflict introduced in 3.8+, where `fl/inplacenew.h` conflicts with GCC 8.4.0's standard `<new>` header on the Xtensa ESP32-S3 toolchain

#### Build flags (`platformio.ini`)
- Removed `-D__AVR__` from `build_flags` — this flag caused FastLED to compile AVR-specific inline assembly using registers `r0`/`r1` which do not exist on the Xtensa core, resulting in a fatal build error

#### ArduinoJson v7 migration
- **`include/DTMPowerCache.h`** — Changed `buildJson()` return type from deprecated `DynamicJsonDocument` to `JsonDocument`; removed stale commented-out duplicate declaration
- **`src/DTMPowerCache.cpp`** — Updated `buildJson()` return type and replaced `DynamicJsonDocument internalDoc(2048)` with `JsonDocument internalDoc` (v7 manages memory dynamically; size argument no longer required)
- **`include/IFTTTAlerts.h`** — Replaced `DynamicJsonDocument doc(1024)` with `JsonDocument doc`; migrated v6 nested creation API to v7 equivalents:
  - `doc.createNestedArray("alerts")` → `doc["alerts"].to<JsonArray>()`
  - `arr.createNestedObject()` → `arr.add<JsonObject>()`

#### ModbusRTUSlave `SoftwareSerial` support (`lib/ModbusRTUSlave`)
- **`src/ModbusRTUSlave.h`** — Added `ModbusRTUSlave(Stream& serial, uint8_t dePin = NO_DE_PIN)` constructor; accepts any `Stream`-derived serial implementation (including `EspSoftwareSerial`) without requiring `__AVR__` to be defined
- **`src/ModbusRTUSlave.cpp`** — Added corresponding `Stream&` constructor implementation; stores the reference in `_serial` with all other serial pointers set to null, consistent with the existing constructor pattern
