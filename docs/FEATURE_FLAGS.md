# Feature Flags / Build Configuration Guide

This document describes how to enable or disable specific features in the EMS firmware using PlatformIO build flags.

## Overview

The project uses PlatformIO's `build_flags` configuration in `platformio.ini` to conditionally compile features. This allows you to:
- Reduce firmware binary size by excluding unused features
- Save RAM by not loading unused code
- Enable only the peripherals you need for your specific deployment
- Avoid compilation errors when hardware is not present

## Configuration

Edit `platformio.ini` and add `-D<FEATURE_NAME>` to the `build_flags` section to enable a feature, or comment it out (with `;`) to disable:

```ini
build_flags =
    ; ... other flags ...
    -DENABLE_OLED_DISPLAY    ; Uncomment to enable
    ; -DENABLE_WIFI          ; Comment to disable (note the ; prefix)
```

**Note:** By default, all feature flags are commented out (disabled) in the template. You need to explicitly enable the features you want.

## Available Feature Flags

| Flag | Description | Dependencies | Default |
|------|-------------|--------------|---------|
| `ENABLE_OLED_DISPLAY` | SH1106 OLED display (SPI) | `SPI` | Disabled |
| `ENABLE_WIFI` | WiFi connectivity | None | Disabled |
| `ENABLE_MQTT` | MQTT client (pub/sub) | `ENABLE_WIFI` | Disabled |
| `ENABLE_CAN` | CAN bus (MCP2515 via SPI) | `SPI` | Disabled |
| `ENABLE_MODBUS_MASTER` | Energy meter subsystem (RS485 Modbus or SPI) | None | Disabled |
| `ENABLE_MODBUS_CLIENT` | Modbus client/slave (RS485_2) | None | Disabled |
| `ENABLE_RELAYS` | Onboard SSR + I2C 8-channel SSR bank (PCF8574) | None | Disabled |
| `ENABLE_DEBUG` | Debug logging to Serial | None | Disabled |

## Meter Type Flags

Set exactly **one** of these flags alongside `ENABLE_MODBUS_MASTER`.  If none is
set, `METER_TYPE_DDS238` is used as the default to preserve backward compatibility.

| Flag | Description | Interface | Channels |
|------|-------------|-----------|---------|
| `METER_TYPE_DDS238` | DDS238 single-phase energy meter (default) | RS-485 Modbus RTU | 3 meters |
| `METER_TYPE_CHD130` | CHINT CHD130 single-phase energy meter | RS-485 Modbus RTU | 3 meters |
| `METER_TYPE_DDSU666` | CHINT DDSU666 single-phase energy meter | RS-485 Modbus RTU | 3 meters |
| `METER_TYPE_ATM90E32` | CircuitSetup 6-channel ATM90E32 meter | SPI | 6 CT channels/board |

### ATM90E32 Additional Configuration

When `METER_TYPE_ATM90E32` is active:

1. **Library**: Uncomment the ATM90E32 library in `lib_deps` in `platformio.ini`:
   ```ini
   https://github.com/CircuitSetup/ATM90E32_Arduino_Library.git
   ```

2. **CS Pins**: Update `ATM90E32_IC1_CS` and `ATM90E32_IC2_CS` in `include/pins.h`
   to match the actual GPIO wiring for your board.  The current values (GPIO 33/34)
   are placeholders.

3. **Board count**: Override the number of board pairs (default = 1 board = 6 channels):
   ```ini
   -DATM90E32_NUM_BOARDS=2   ; 12 channels across 2 board pairs
   ```

4. **Calibration**: Override defaults at build time if your hardware differs:
   ```ini
   -DATM90E32_LINE_FREQ=135          ; 50 Hz (default 4231 = 60 Hz)
   -DATM90E32_VOLTAGE_GAIN=42080     ; 9V Jameco 112336 (meter <= v1.2)
   -DATM90E32_CURRENT_GAIN=11131     ; 20A/25mA SCT-006 CT
   -DATM90E32_PGA_GAIN=21            ; 2× gain for low-output CTs
   ```

5. **readings[] layout**: Each channel maps to one `readings[]` slot:
   - `readings[0..2]` = IC1 channels A/B/C (CT1–CT3)
   - `readings[3..5]` = IC2 channels A/B/C (CT4–CT6)
   - Additional boards extend to `readings[6..11]`, etc.

### Modbus Meter Address Configuration

For Modbus meters (DDS238 / CHD130 / DDSU666), the three meter Modbus node
addresses are set in `modbus_master.cpp`:

```cpp
#define METER_1_ADDR 0x50
#define METER_2_ADDR 0x51
#define METER_3_ADDR 0x52
```

Programme each meter to its assigned address before installation using the
meter's front-panel configuration interface.

## Examples

### Minimal Setup (UART + Buttons only)
```ini
build_flags =
    -DUSE_SOFTWARE_SERIAL
    -DBOARD_VER_V3
    ; No features enabled - just buttons and serial debug
```

### Basic Monitoring (Display + Buttons)
```ini
build_flags =
    -DUSE_SOFTWARE_SERIAL
    -DBOARD_VER_V3
    -DENABLE_OLED_DISPLAY
    -DENABLE_DEBUG
```

### WiFi Monitoring (Display + WiFi + MQTT)
```ini
build_flags =
    -DUSE_SOFTWARE_SERIAL
    -DBOARD_VER_V3
    -DENABLE_OLED_DISPLAY
    -DENABLE_WIFI
    -DENABLE_MQTT
    -DENABLE_DEBUG
```

### Full Metering (All features)
```ini
build_flags =
    -DUSE_SOFTWARE_SERIAL
    -DCONFIG_ETH_ENABLED
    -DENABLE_RGBWLED
    -DBOARD_VER_V3
    -DENABLE_OLED_DISPLAY
    -DENABLE_WIFI
    -DENABLE_MQTT
    -DENABLE_CAN
    -DENABLE_MODBUS_MASTER
    -DENABLE_MODBUS_CLIENT
    -DENABLE_RELAYS
    -DENABLE_DEBUG
```

### Modbus Only (No display, no WiFi)
```ini
build_flags =
    -DUSE_SOFTWARE_SERIAL
    -DBOARD_VER_V3
    -DENABLE_MODBUS_MASTER
    -DENABLE_MODBUS_CLIENT
    -DENABLE_DEBUG
```

## Automatic Dependencies

Some features automatically enable their dependencies:

- **`ENABLE_MQTT`** automatically enables `ENABLE_WIFI` (MQTT requires WiFi)
  - If you enable `ENABLE_MQTT` without `ENABLE_WIFI`, WiFi will be enabled automatically

## Board Configuration

In addition to feature flags, make sure your board revision is correctly set:

```ini
; Uncomment exactly ONE of these:
-DBOARD_VER_V1    ; Board version 1
-DBOARD_VER_V2    ; Board version 2  
-DBOARD_VER_V3    ; Board version 3 (ESP32-S3 DevKit)
```

## Verifying Your Configuration

After building, check the serial output at boot. You should see messages like:

```
INFO - Booting
INFO - Debug logging enabled
Display up! next is WiFi/Eth, NTP, MQTT, Modbus, Buttons...
[...]
INFO - Debug logging enabled
```

Features that are disabled will not produce any output or consume resources.

## Troubleshooting

### Feature not working after enabling
1. Make sure you rebuilt the project: `pio run` or click Build in VSCode
2. Check for compilation errors in the build output
3. Verify the feature's hardware dependencies are connected
4. Check `platformio.ini` for the correct board revision

### Compilation errors
Some features depend on specific header files. If you get errors like `'display.h' file not found`:
1. Verify the include paths are correct
2. Check that required libraries are installed (see `platformio.ini`)
3. Make sure related feature flags are enabled (e.g., `ENABLE_OLED_DISPLAY` requires `SPI`)

### Binary too large
ESP32-S3 has 16MB flash, but you can still run out of space with all features enabled plus debug logging. To reduce size:
1. Disable features you don't need
2. Disable `ENABLE_DEBUG` (removes debug strings)
3. Disable `ENABLE_OLED_DISPLAY` if not using a display

### RAM usage too high
Features like WiFi and MQTT consume significant RAM. Consider:
1. Disabling unused features
2. Reducing MQTT publish rates
3. Not using the OLED display

## Runtime Behavior

### Disabled Features
- No memory is allocated for disabled features
- No CPU cycles are consumed
- Setup functions are not called
- Loop functions are not executed
- Header files are not included (reducing compile time)

### Enabled Features
- All necessary initialization is performed
- Background tasks run as expected
- Resources (SPI, I2C, GPIO) are claimed
- Data structures are allocated

## Integration with Other Configuration

The feature flags work alongside other PlatformIO build options:

```ini
build_flags =
    ; Feature flags
    -DENABLE_OLED_DISPLAY
    -DENABLE_WIFI
    -DENABLE_MQTT
    
    ; Board configuration
    -DBOARD_VER_V3
    
    ; Other options
    -DUSE_SOFTWARE_SERIAL
    -UARDUINO_USB_CDC_ON_BOOT
```

## Best Practices

1. **Start minimal**: Enable only what you need, add features incrementally
2. **Test after each change**: Rebuild and test when enabling new features
3. **Monitor serial output**: Check for boot messages confirming feature status
4. **Document your config**: Comment your `platformio.ini` to explain why features are enabled
5. **Version control**: Commit changes to `platformio.ini` with clear messages

## Advanced: Conditional Code in Your Projects

You can use the same feature flags in your own code:

```cpp
#ifdef ENABLE_WIFI
  // WiFi-specific code
  setup_wifi();
#endif

#ifdef ENABLE_DEBUG
  Serial.println("Debug message");
#endif
```

This ensures your custom code is only compiled when the corresponding feature is enabled.

## Feature Interdependencies

```
Basic System
    ├── Buttons (always enabled)
    ├── Config (always enabled)
    └── Data Model (always enabled)
        ├── Display
        │   └── Console UI
        ├── WiFi
        │   └── MQTT (requires WiFi)
        ├── CAN Bus (separate SPI instance)
        ├── Modbus Master (RS485_1)
        ├── Modbus Client (RS485_2)
        ├── I2C SSR Bank
        └── Relays
```

## Troubleshooting Checklist

- [ ] `platformio.ini` has correct `build_flags`
- [ ] Board revision (`BOARD_VER_*`) is set
- [ ] Project has been rebuilt (`pio run`)
- [ ] Serial output shows expected boot messages
- [ ] Hardware is properly connected
- [ ] No compilation errors
- [ ] Feature dependencies are met (e.g., WiFi for MQTT)
- [ ] SPI/I2C pins are not conflicting (check `pins.h`)

## Support

For issues related to feature flags:
1. Check this guide
2. Review `platformio.ini` for correct syntax
3. Examine serial output for boot messages
4. Verify hardware connections match enabled features
5. Check compilation output for errors

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-04-25 | Initial feature flag system documentation |
