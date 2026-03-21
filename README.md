# BLEewerLite

An ESPHome external component for controlling Neewer LED lights via Bluetooth Low Energy. It turns your ESP32 into a BLE-to-Home Assistant bridge, exposing Neewer lights as native light entities with brightness, color temperature, RGB, and animation effect support.

The protocol implementation is based on the excellent [NeewerLite-Python](https://github.com/taburineagle/NeewerLite-Python) project by Zach Glenwright. Without that reverse engineering effort, none of this would exist. If you need a standalone desktop controller for your Neewer lights, go use that instead. It is better.

Runs entirely local on an ESP32. No cloud, no phone app, no account. Just BLE and Home Assistant.

## Quick Start

1. Find your Neewer light's BLE MAC address (see [Finding your light's MAC address](#finding-your-lights-mac-address))
2. Add the component to your ESPHome device YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/mplogas/bleewerlite-esphome
    components: [bleewer_light]

esp32_ble_tracker:
  scan_parameters:
    active: true
    interval: 200ms
    window: 180ms

ble_client:
  - mac_address: "AA:BB:CC:DD:EE:FF"
    id: neewer_light_1

light:
  - platform: bleewer_light
    ble_client_id: neewer_light_1
    name: "Neewer Key Light"
    default_transition_length: 0s
```

3. Compile and flash: `esphome compile your-config.yaml && esphome upload your-config.yaml`
4. The light should appear in Home Assistant once the ESP32 connects via BLE.

## Configuration

### Required

| Option | Description |
|--------|-------------|
| `ble_client_id` | Reference to a `ble_client` entry with the light's MAC address |
| `name` | Entity name in Home Assistant |
| `default_transition_length: 0s` | **Do not remove this.** Neewer lights choke on rapid BLE command streams. ESPHome's default transition sends many intermediate values that will overwhelm the light. |

### Optional

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `model_name` | string | (none) | Model identifier for auto-detection of capabilities. Matched against the built-in light specs database (e.g. `"GL1"`, `"RGB660"`, `"SL90"`). |
| `color_temp_min` | int | 3200 | Minimum color temperature in Kelvin. Auto-detected from `model_name` if set. |
| `color_temp_max` | int | 5600 | Maximum color temperature in Kelvin. Auto-detected from `model_name` if set. |
| `cct_only` | bool | false | Set to `true` for lights that only support color temperature (no RGB). Auto-detected from `model_name` if set. |
| `infinity_mode` | int | 0 | Protocol variant: 0 = standard, 1 = infinity, 2 = hybrid. Auto-detected from `model_name` if set. |

### Animation Effects

Neewer lights have 17 built-in hardware animation effects. These run on the light itself; the ESP32 just sends the trigger command. Add them to your config to make them available in Home Assistant:

```yaml
light:
  - platform: bleewer_light
    ble_client_id: neewer_light_1
    name: "Studio Light"
    default_transition_length: 0s
    effects:
      - neewer_lightning:
      - neewer_paparazzi:
      - neewer_defective_bulb:
      - neewer_explosion:
      - neewer_welding:
      - neewer_cct_flash:
      - neewer_hue_flash:
      - neewer_cct_pulse:
      - neewer_hue_pulse:
      - neewer_cop_car:
      - neewer_candlelight:
      - neewer_hue_loop:
      - neewer_cct_loop:
      - neewer_intensity_loop:
      - neewer_tv_screen:
      - neewer_fireworks:
      - neewer_party:
```

Each effect accepts optional parameters in YAML. All effects support `brightness` (0-100, default 100) and `speed` (1-10, default 5). Some effects have additional parameters:

```yaml
effects:
  - neewer_candlelight:
      brightness_min: 10
      brightness_max: 60
      color_temp: 32        # 32 = 3200K
      speed: 3
  - neewer_cop_car:
      special_options: 2    # 1-3
  - neewer_hue_flash:
      hue: 120              # 0-360
      saturation: 100       # 0-100
```

Effects respect the current Home Assistant brightness slider. Setting brightness below 16% will clamp to 16%, as Neewer lights ignore effects at very low brightness levels.

### Multi-Light Setup

```yaml
ble_client:
  - mac_address: "AA:BB:CC:DD:EE:FF"
    id: key_light
  - mac_address: "11:22:33:44:55:66"
    id: fill_light

light:
  - platform: bleewer_light
    ble_client_id: key_light
    name: "Key Light"
    model_name: "GL1"
    default_transition_length: 0s
  - platform: bleewer_light
    ble_client_id: fill_light
    name: "Fill Light"
    default_transition_length: 0s
    effects:
      - neewer_cop_car:
      - neewer_candlelight:
```

## Tested Devices

### Neewer Lights

| Model | Protocol | Color Modes | BLE Address Type | Status |
|-------|----------|-------------|------------------|--------|
| GL1-Pro | Standard, CCT-only | Brightness + Color Temp (2900-7000K) | Random | Working. Slow BLE advertising (~5 min intervals). |
| HS60C | Standard, RGB+CCT | Brightness + Color Temp + RGB + Effects | Public | Working. Fast advertising, reliable. |
| HB80C | Infinity (mode 1), RGB+CCT | Brightness + Color Temp (2500-7500K) + RGB + Effects | Random | Working. Also accepts standard commands. |

The component includes a specs database with 48 known Neewer models. Lights not in the database fall back to sensible defaults (3200-5600K, RGB+CCT, standard protocol). If your light works with [NeewerLite-Python](https://github.com/taburineagle/NeewerLite-Python), it should work here.

### ESP32 Boards

| Board | Status | Notes |
|-------|--------|-------|
| ESP32-S3-DevKitC-1 | Tested | Dual USB (JTAG + UART) is convenient for development. |
| ESP32-WROOM-32 | Tested | Use `board: esp32dev` in config. |
| LilyGO T-Internet-POE | Tested | Ethernet + BLE. Single PoE cable, no WiFi/BLE contention. Use `board: esp32dev` with `ethernet:` config. |
| Seeed XIAO ESP32-C3 | Tested | Use `board: seeed_xiao_esp32c3`. Requires external antenna. |
| Seeed XIAO ESP32-S3 | Tested | Use `board: seeed_xiao_esp32s3`. Requires external antenna. |
| ESP8266 (D1 Mini, etc.) | Not compatible | No BLE hardware. |

Any ESP32 variant with BLE support should work. The firmware uses about 1.2MB flash and 50KB RAM with two lights configured.

## Finding Your Light's MAC Address

### Option 1: BLE Scanner

Use any BLE scanner app on your phone. Look for devices named `NEEWER-*`, `NW-*`, or `SL*`.

### Option 2: bluetoothctl (Linux)

```bash
bluetoothctl scan on
# Look for NEEWER-* entries. Ctrl+C when found.
```

Note: some Neewer lights use BLE random addresses and have slow advertising intervals. If `bluetoothctl` does not find your light, try a longer scan or use Python with bleak:

```bash
pip install bleak
python3 -c "
import asyncio
from bleak import BleakScanner
async def scan():
    scanner = BleakScanner(detection_callback=lambda d, a: print(f'{d.address} {d.name}') if d.name and 'NEEWER' in d.name.upper() or 'NW-' in (d.name or '') else None)
    await scanner.start()
    await asyncio.sleep(120)
    await scanner.stop()
asyncio.run(scan())
"
```

### Option 3: ESPHome BLE Tracker

Flash a minimal ESPHome config with just `esp32_ble_tracker` and check the logs for Neewer devices.

## Limitations

- **BLE only.** This component controls Neewer lights over Bluetooth Low Energy. It does not support Neewer's WiFi protocol (used by the Neewer Live app and 2.4G dongle). If your light only has WiFi and no BLE, this will not work.
- **3 lights per ESP32.** The ESP32 BLE stack supports approximately 3 concurrent client connections. This is a hardware constraint. For more lights, use multiple ESP32 boards.
- **No state readback.** The component tracks power state internally but cannot reliably read the light's current settings. If you change settings using the Neewer app or the light's physical controls, the Home Assistant state will be out of sync.
- **Slow advertising on some lights.** Some Neewer lights (notably the GL1-Pro) advertise infrequently via BLE, sometimes only every 5 minutes. Initial connection after boot can take a while. Once connected, the link is stable.
- **No BLE security.** The Neewer BLE protocol has no authentication, encryption, or pairing. Anyone within BLE range (~10 meters) can control your lights. This is how Neewer designed it. If this concerns you, consider the threat model: someone standing outside your studio toggling your key light.

## Architecture

```
components/bleewer_light/
  __init__.py              ESPHome component registration
  light.py                 Light platform config schema and codegen
  effects.py               Animation effect registration (17 effects)
  neewer_protocol.h/.cpp   Protocol encoder: packet building, checksum, light specs DB
  neewer_effects.h/.cpp    NeewerEffect class: sends ANM command to light on start
  bleewer_light.h/.cpp     BLE GATT client + ESPHome LightOutput integration
```

The protocol layer (`neewer_protocol`) has zero ESPHome dependencies and could be reused in other contexts. The BLE integration layer (`bleewer_light`) handles GATT lifecycle, command queuing with rate limiting (50ms between writes), and maps ESPHome light state to protocol commands.

## Credits

- [NeewerLite-Python](https://github.com/taburineagle/NeewerLite-Python) by Zach Glenwright. The protocol reference, the light specs database, the name correction map, and frankly the entire understanding of how these lights work over BLE comes from this project. Go star it.
- [ESPHome](https://esphome.io/) for the framework that makes building custom Home Assistant integrations almost pleasant.

## License

MIT
