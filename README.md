# EIOT.Energy EMS DER/Site Controller Dev Kit - OpenAMI Metering Application

## Overview
A development kit based on the ESP32S3 N16R8 DEV KIT C1 for energy management systems (EMS) with support for various communication protocols and peripherals. There are a few N16R8 40/42/44 pin layouts. Th EMS kit variant where the RGVW led is top center mounted just below the WROOM ESP32S3 surface mount module. All variants will work except the pins layout differs. 

### Current Status

Active build flags (see `platformio.ini`): `ENABLE_OLED_DISPLAY`, `ENABLE_WIFI`, `ENABLE_MQTT`, `ENABLE_MODBUS_MASTER`, `ENABLE_DEBUG`, `METER_TYPE_ATM90E32`.

| Subsystem | Status | Notes |
|---|---|---|
| WiFi / MQTT | ✅ Working | Connects to broker, publishes all topics |
| ATM90E32 6-Ch SPI Meter | ⚠️ Implemented / Not field-tested | New driver; CS pins GPIO 33/34 are placeholders — verify against board schematic before use. See known issue below. |
| SHT20 Temp/Humidity (Modbus) | ❌ Timeout | SoftwareSerial / WiFi interrupt contention. See known issue below. |
| DDS238 / CHD130 / DDSU666 (Modbus) | ⚠️ Supported, inactive | Addresses corrected to `0x01–0x03`. Enable by setting `METER_TYPE_DDS238` (or variant) in `platformio.ini`. |
| I2C SSR Bank (PCF8574) | ⚠️ Implemented, disabled | `ENABLE_RELAYS` is commented out in `platformio.ini`. Uncomment to activate. |
| Onboard SSR (GPIO38) | ⚠️ Implemented, disabled | Controlled by `ENABLE_RELAYS`; same flag as I2C SSR bank. |
| MCP2515 CAN | ❌ Disabled / Init failure | `ENABLE_CAN` commented out. When enabled: `Entering Configuration Mode Failure` — SPI wiring or crystal freq mismatch. See known issue below. |

### Known Issues

#### ATM90E32 CS Pin Placeholders

`include/pins.h` defines `ATM90E32_IC1_CS = GPIO 33` and `ATM90E32_IC2_CS = GPIO 34` as placeholders. These **must** be verified against the actual NESL 865B board schematic before field use. Wrong CS pins will cause silent SPI failures (the ICs will not respond, and all readings will be zero).

**Fix:** Check the schematic for the CS lines routed to the ATM90E32 headers and update `ATM90E32_IC1_CS` / `ATM90E32_IC2_CS` in `include/pins.h` accordingly.

---

### IOT MUG 8-Channel I2C SSR Board

**Product:** [IOT MUG 8-Channel I2C Solid State Relay Module](https://www.iotmug.com/8-channel-i2c-solid-state-relay-module)

#### I2C Address
The board uses **0x27** (all address DIP switches ON) or **0x3F** (PCF8574A variant). Check the red DIP switch block on the board — the default out of box is **0x27**. Set `PCF8574_I2C_ADDR` in `include/pins.h` to match.

#### Wiring (BOARD_VER_V3)
| SSR Board Pin | EMS Board Connection | GPIO |
|---|---|---|
| VCC | 5V | — |
| GND | GND | — |
| SDA | SSR2 header | GPIO 14 |
| SCL | SSR1 header | GPIO 20 |

`I2C_SSR_SDA_GPIO` and `I2C_SSR_SCL_GPIO` in `include/pins.h` must match the physical wiring. Avoid GPIO46 — it has an internal pull-down that locks up the I2C bus.

#### Channel Mapping
The PCF8574 P0 bit maps to **relay 8** on the IOT MUG board (reversed order):

| Bit | Relay |
|---|---|
| P7 (bit 7) | Relay 1 |
| P6 (bit 6) | Relay 2 |
| ... | ... |
| P0 (bit 0) | Relay 8 |

The firmware uses active-low logic (`I2C_SSR_ACTIVE_LOW 1`) which matches the IOT MUG board.

---

### Upload & Serial Monitor (macOS)

The ESP32-S3 DevKitC-1 has **two USB-C ports**:
- **COM port** (via CH343P UART bridge) — use this for both flashing and serial monitor
- **USB port** (native ESP32-S3 USB) — not needed for normal dev use

Always use the **COM port**. The firmware has `ARDUINO_USB_CDC_ON_BOOT` disabled so `Serial` output goes to UART0 (COM port), keeping flash and monitor on the same connector.

If upload fails with "Resource busy", a `screen` session may be holding the port:
```bash
lsof /dev/cu.usbmodem*   # find the PID
kill <PID>
```

Flash command:
```bash
pio run -t upload --upload-port /dev/cu.usbmodemXXXXX
```

Serial monitor:
```bash
screen /dev/cu.usbmodemXXXXX 115200
# Exit: Ctrl+A, K, Y
```

## Features
This development kit supports multiple peripherals using the PlatformIO and Arduino framework:
- RS-485 MODBUS RTU communication (DDS238, CHD130, DDSU666 single-phase meters)
- CircuitSetup ATM90E32 6-channel SPI energy meter (3-phase / multi-CT)
- CANBUS V2.0 interface via SPI (MCP2515)
- Input buttons (using voltage divider array on analog GPIO)
- 1.3in OLED Display over SPI (SH1106)
- Feature-flag build system — compile only the subsystems you need (see `FEATURE_FLAGS.md`)

All optional subsystems are guarded by `#ifdef` feature flags set in `platformio.ini`.  See [`FEATURE_FLAGS.md`](FEATURE_FLAGS.md) for the full flag reference and example configurations.

## Board Diagrams

### NESL 865B EMS Board

<img src="NESL%20865B_EMS_Board_Diagram.jpg" alt="NESL 865B EMS Board Diagram" width="700"/>

### NESL 865B EMS Board — with CircuitSetup ATM90E32 6-Channel Meter

<img src="NESL%20865B_EMS_Board_Diagram-wCircuitSetup.jpg" alt="NESL 865B EMS Board Diagram with CircuitSetup ATM90E32" width="700"/>

### Legacy MeshEMS Board (V001) built in 2025

<img src="ems_board_pinout_V001.png" alt="EMS Board Pinout V001" width="650"/>

## Hardware Overview

### Core Specifications
- **Processor:** Xtensa® dual-core 32-bit LX7 microprocessor, up to 240 MHz
- **Memory:** 16MB Flash + 8MB PSRAM (N16R8 variant)
- **Connectivity:** Wi-Fi 802.11 b/g/n and Bluetooth 5 (LE)
- **USB:** USB OTG interface with Type-C connector
- **GPIO:** 45 programmable GPIO pins
- **Dimensions:** 51mm x 25.5mm x 10mm
- **Operating Voltage:** 3.3V
- **Datasheet:** [ESP32S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- **Development Board Datasheet:** [ESP32S3-DevKitC-1 Datasheet](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html)

### HW-519 Breakout RS-485 MODBUS RTU Module
- Industry-standard RS-485 interface for MODBUS RTU communication
- Built-in transceiver with automatic direction control
- 3-pin screw terminal for easy connection (A, B, GND)
- Supports baud rates up to 115200 bps
- **Operating voltage:** 5V (level-shifted from ESP32-S3 at 3.3V)
- **Module Datasheet:** [RS-485 Transceiver Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX1487-MAX491.pdf)

### MCP2515 Breakout - CANBUS V2.0 Interface
- CAN 2.0B compliant controller and transceiver
- Supports standard (11-bit) and extended (29-bit) identifiers
- Maximum bitrate: 1 Mbit/s
- Screw terminals for CANH and CANL connections
- Integrated termination resistors (jumper selectable)
- **Controller Datasheet:** [MCP2515 CAN Controller](https://ww1.microchip.com/downloads/en/DeviceDoc/MCP2515-Stand-Alone-CAN-Controller-with-SPI-20001801J.pdf)
- **Transceiver Datasheet:** [TJA1051 CAN Transceiver](https://www.nxp.com/docs/en/data-sheet/TJA1051.pdf)

### Additional Communication Options
- **BLE/BLE Mesh:** Utilizing ESP32S3's built-in Bluetooth capabilities

### Input/Output Capabilities
- **Button Array Interface:** Analog input with voltage divider network
- **Display:** Optional 1.3" OLED display (SPI interface)
- **Expansion Headers:** Breakout area is available on the perfboard to allow for use of the remaining GPIO pins
- **Relay:** Single 2amp SSR (Solid State Relay) for AC Mains control

### Power Supply Options
- **USB Power:** 5V via USB Type-C connector
- **DC Power:** 5VDC via screw terminals to on board, connects directly to 5VIN of ESP32 (5V input MAX).
- **AC Power:** ⚠️ 120VAC input via screw terminals on board to power supply. 

### ⚠️ WARNING: AC Power Safety
### DANGER - RISK OF ELECTRIC SHOCK, SERIOUS INJURY OR DEATH
This development kit includes a connection for AC power input. When working with AC power (especially 120V/240V mains voltage):

- **Professional Installation Required:** All AC power connections MUST be installed by a qualified electrician in accordance with local electrical codes and regulations.
- **Enclosure Mandatory:** When used with AC power connections, the device MUST be mounted in an appropriate, non-conductive enclosure with restricted access.
- **Safety Precautions:**
- **ALWAYS disconnect AC power** before making any changes to the wiring
- **NEVER touch any AC terminals** or components when power is connected
- Ensure proper grounding of all components
- Install appropriate circuit protection (fuses, breakers)
- Keep AC and DC/logic circuits strictly separate
**Not UL/CE Certified for AC Applications:** This development kit by itself is NOT certified for direct connection to AC mains.

**⚠️ Failure to follow these safety guidelines could result in severe electrical shock, fire, serious injury, or death. ⚠️**

### Physical Specifications
- PCB Dimensions: 150mm x 90mm (main board)
- Mounting: 4x M3 mounting holes (3.2mm diameter)

## Dev Environment Installation Guide
### Prerequisites
- A computer with internet connection
- EMS Dev kit hardware
- USB-C data cable for connecting the development board to your computer

### Step 1: Install Visual Studio Code
1. Download Visual Studio Code from https://code.visualstudio.com/
2. Follow the installation instructions for your operating system:
  - **Windows:** Run the installer and follow the prompts
  - **macOS:** Drag the application to your Applications folder

### Step 2: Install PlatformIO Extension

1. Open VSCode
2. Click on the Extensions icon in the left sidebar (or press Ctrl+Shift+X)
3. Search for "PlatformIO IDE"
4. Click "Install" on the PlatformIO IDE extension
5. Wait for the installation to complete (this may take a few minutes)
6. Restart VSCode when prompted

### Step 3: Clone the Repository
1. Open a terminal/command prompt
2. Navigate to the directory where you want to store the project
3. Clone the repository using git:
4. `git clone https://github.com/energy-iot/meshems-openami-metering.git`
5. `git checkout <your-feature-branch>
6. Follow steps 4 and 5.
7. Use `git commit -s` to sign your Pull Request commits.

### Step 4: Open the Project in VSCode
1. In VSCode, click on the PlatformIO icon in the left sidebar
2. Select "Open Project" from the PlatformIO home screen
3. Navigate to the cloned repository folder and select it
4. Wait for VSCode to load the project and initialize PlatformIO

### Step 5: Configure the Project
1. Wait for PlatformIO to download all required dependencies (libraries)
  **IMPORTANT:** Set the environment to ESP32S3 N16R8 DEV KIT C
  - Open the platformio.ini file in the project root
  - Make sure the environment section contains [env:esp32-s3-devkitc-1 , esp32s3_n16r8] or similar
  - If not, add or modify the environment section to match the ESP32S3 N16R8 DEV KIT C
  - select a latest working branch of project for example visualize vs openami 3phase mqtt vs future others (leakage, diagnostics, etc)

### Step 6: Build and Flash the Firmware
1. Connect your ESP32S3 DEV KIT to your computer via USB-C
2. In VSCode, click on the PlatformIO icon in the left sidebar
3. Select "Project Tasks" from the menu
4. Under "General", click "Build" to compile the project
5. After successful build, click "Upload" to flash the firmware to your device
6. Monitor the progress in the terminal window at the bottom of VSCode

### Troubleshooting

- If you encounter upload errors, ensure that:
  - The correct USB port is selected (can be changed in platformio.ini)
  - You have proper USB drivers installed for your development board
  - Your board is in bootloader mode (if required)
- Check the PlatformIO documentation for additional help: https://docs.platformio.org/


### Front-of-Meter IEEE ISV StreetPoleEMS Integrations

#### Framework & Networking
- Front-of-Meter OPENAMI bidirectional monitor and control PUB/SUB framework
- on board OLED real time energy visualizer graphing and webserver
- StreetPoleEMS MESH distributed intelligence Pub/Sub networking
- Linux Aggregation FLEXMEASURES policy layering export policy enforcement schedules to ESP32S3 EMS - see proposed HLD in Google docs
- EMS MESH networking to N:1 StreetPoleEMS Linux Node Aggregator with distributed AI Energy Policy, N=10~100

#### Metering Plugins
- IVY Metering Bidirectional AC/DC powerflow RCD and RVD leakage modbus monitoring, alarm lines detection
- Donsun DLMS/STS prepaid meter integration
- Donsun postpaid meter integration
- IVY Metering AC/DC meter prepaid/postpaid integration (DLMS/STS)
- Modbus RTU AC Energy Meter plugins (Single Phase, Split Phase, Three Phase)
- Bidirectional AC/DC powerflow RCD and RVD leakage detection
- EVSE AC/DC Charge/Discharge controller plugins
- VFD Modbus RTU plugin
- Energy IoT device plugin

#### Additional Integrations
- ENACCESS OPEN SMART METER libraries for Paygo Dongle support
- OPENPLC integration with IFTTT Rules engine and Modbus PLC endpoint support
- 18650 battery backup with state persistence to flash memory
- RTC clock with sleep and deep sleep power management

### Extended Networking Capabilities
- Ethernet MAC/PHY WAN/LAN dual port
- BLE mesh LAN networking
- G3 ALLIANCE RF+PLC MAC/PHY module for WAN/LAN mesh networking
- LR BLE mesh WAN networking
- LORA Meshtastic LAN/WAN networking
- LORAWAN WAN connectivity
- LTE CATM1 global SIM/eSIM radio module
- Starlink MAC/PHY WWAN radio module integration
- DRONE passby secure ondemand  BLE/wifi  networking, OTA, config, restoration

## Contributing
Check the https://github.com/energy-iot/docs Feel free to suggest additional integration ideas via a pull request or contribute to existing challenges.
