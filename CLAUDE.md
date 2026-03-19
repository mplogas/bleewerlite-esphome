# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BLEewerLite is an ESPHome external component for controlling Neewer LED lights via Bluetooth Low Energy. It acts as a BLE GATT client running on ESP32 hardware, exposing Neewer lights as Home Assistant light entities with brightness, color temperature, RGB, and animation effect support.

The protocol is reverse-engineered from [NeewerLite-Python](https://github.com/taburineagle/NeewerLite-Python).

## Building & Testing

This is an ESPHome external component, not a standalone project. To use:

```yaml
# In an ESPHome device YAML:
external_components:
  - source:
      type: local
      path: /path/to/BLEewerLite/components
    components: [bleewer_light]
```

To compile and flash (venv must be active):
```bash
esphome compile example.yaml
esphome upload example.yaml
esphome logs example.yaml
```

There is no standalone test harness yet. Protocol logic should be validated by comparing output bytearrays against NeewerLite-Python's `calculateByteString()`.

## Architecture

### Component Structure

```
components/bleewer_light/
├── __init__.py              # ESPHome component registration, YAML config schema
├── light.py                 # Light platform definition (Python codegen)
├── effects.py               # Animation effect registration (17 effects with YAML params)
├── neewer_protocol.h/.cpp   # Pure protocol layer: packet building, checksum, light specs DB
├── neewer_effects.h/.cpp    # NeewerEffect class: sends ANM command on start, no-op apply
├── bleewer_light.h/.cpp     # BLE communication + ESPHome light integration
```

### Separation of Concerns

- **`neewer_protocol`**: Zero BLE/ESPHome dependencies. Takes parameters, returns `std::vector<uint8_t>` bytearrays. Contains light specs database and name correction. Testable in isolation.
- **`bleewer_light`**: Subclasses `ble_client::BLEClientNode` and `light::LightOutput`. Handles GATT lifecycle, maps ESPHome light state to protocol commands, manages connection and rate-limiting.

### Protocol Variants

Three protocol variants determined per-light by `infinity_mode` from the light specs database:

| Mode | Variant | Key Difference |
|------|---------|----------------|
| 0 | Standard | Direct commands to write characteristic |
| 1 | Infinity | Commands wrapped with 6-byte hardware MAC address |
| 2 | Infinity-hybrid | Uses Infinity FX byte (0x8B) but no MAC wrapping, 3-param CCT |

**CCT-only lights** (where `cct_only = true` in the specs DB) use **separate brightness (0x82) and color temp (0x83) commands** instead of the combined CCT command (0x87). This is critical — the combined command is silently ignored by these lights. In NeewerLite-Python, this maps to the `availableLights[x][5]` flag, which is set from the `cct_only` field in the specs DB.

### BLE Details

- **Service UUID**: `69400001-B5A3-F393-E0A9-E50E24DCCA99`
- **Write Characteristic**: `69400002-B5A3-F393-E0A9-E50E24DCCA99`
- **Notify Characteristic**: `69400003-B5A3-F393-E0A9-E50E24DCCA99`

### Key Constraints

- Neewer lights are fragile under rapid BLE writes. Always set `default_transition_length: 0s`. The component queues commands with a 50ms minimum interval between writes.
- **ESP32 supports ~3 concurrent BLE client connections.** This is a hardware limitation — plan one ESP32 per ~3 lights. Higher connection counts are possible with config tuning but RAM-limited and unreliable.
- Default BLE MTU is 23 bytes (~20 payload). All Neewer commands fit within this.
- ESPHome uses mireds for color temperature (1000000 / Kelvin). The protocol uses raw byte values where e.g. 32 = 3200K, 56 = 5600K.
- Some Neewer lights use **BLE random addresses**. ESPHome's BLE tracker auto-detects the address type, but these lights may have slow advertising intervals (~5 minutes between advertisements).
- `esphome clean` is required when modifying component C++ source — ESPHome's incremental build may not detect external component changes.
- ESPHome components must be in `namespace esphome { namespace bleewer_light { ... }}`, not a standalone namespace.

### Tested Hardware

| Light | Protocol | BLE Address Type | Notes |
|-------|----------|------------------|-------|
| GL1-Pro | Standard, CCT-only (separate 0x82/0x83) | Random | Slow advertising (~5 min intervals) |
| HS60C | Standard, RGB+CCT (0x87/0x86), effects via 0x8B | Public | Fast advertising, reliable |

## Protocol Reference

All packets: `[0x78, CMD, LEN, ...PARAMS, CHECKSUM]`
Checksum: `sum(all_bytes_except_checksum) & 0xFF`

### Command Table

| CMD | Mode | Payload | Notes |
|-----|------|---------|-------|
| `0x81` | Power | `[on_off]` (1=ON, 2=OFF) | Works on all lights |
| `0x82` | Brightness (old) | `[brightness]` | CCT-only lights only |
| `0x83` | Color temp (old) | `[temp]` | CCT-only lights only |
| `0x86` | HSI | `[hue_lo, hue_hi, sat, bri]` | RGB lights |
| `0x87` | CCT | `[bri, temp]` | Standard RGB+CCT lights (mode 0, non-cct_only) |
| `0x88` | ANM (legacy) | `[bri, fx_index]` | Very old lights only — requires FX index conversion |
| `0x8B` | ANM/Scene | `[effect, ...params]` | **Use this for effects on all modern lights** (tested on HS60C) |

### Animation Effects (0x8B command)

Effects are sent once — the light runs the animation internally. 18 effects supported, each with different parameter layouts. Key finding: **`0x8B` works on standard (mode 0) lights**, not just Infinity/hybrid. The legacy `0x88` command with FX index conversion only works on very old lights.

Effect parameter layouts match NeewerLite-Python's `calculateByteString()` ANM section (lines 3056-3103). Parameters vary per effect (brightness, speed, temp, hue, saturation, sparks, special_options, etc.).

ESPHome integration: effects are registered via `register_monochromatic_effect` in `effects.py` and implemented as `NeewerEffect` subclasses in `neewer_effects.h/.cpp`. The `apply()` method is a no-op since the light handles animation internally — the command is sent once in `start()`. `write_state()` skips color/brightness commands while an effect is active (`get_current_effect_index() != 0`).

## Coding Conventions

- C++ files use ESPHome conventions: `snake_case` for methods/variables, `PascalCase` for classes
- Use ESPHome logging macros: `ESP_LOGD`, `ESP_LOGW`, `ESP_LOGI`, `ESP_LOGV`
- Tag all logs with `TAG` constant: `static const char *TAG = "bleewer_light";`
- Python config files use `esphome.codegen` (cg) and `esphome.config_validation` (cv) APIs
- Keep protocol logic in `neewer_protocol.*`, BLE/ESPHome integration in `bleewer_light.*`
