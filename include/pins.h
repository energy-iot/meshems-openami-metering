/*****************************************************************************
 * @file pins.h
 * @brief Pin definitions for dev board
 * 
 * This header defines all GPIO pin assignments for the system peripherals:
 * - Analog button array (using voltage divider)
 * - SPI OLED display (SH1106)
 * - RS-485 interfaces (dual channels using HW-519 modules)
 * - Relay control
 * - CAN bus interface (MCP2515)
 * 
 * 
 * Author(s): Doug Mendonca, Liam O'Brien
 *****************************************************************************/

#pragma once

// Board Version 1 - 2025 
#if defined(BOARD_VER_V1)
    // analog button array (voltage divider)
    #ifdef CONFIG_IDF_TARGET_ESP32S3
        #define ANALOG_BTN_PIN  A0 //GPIO1
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ANALOG_BTN_PIN  A0
    #else
        #define ANALOG_BTN_PIN  A7
    #endif

    // ==================== SPI DISPLAY ====================
    #define DISPLAY_RST_PIN 46  //Reset
    #define DISPLAY_DC_PIN 3    //Data clock
    #define DISPLAY_CS_PIN 9    //Chip select

    // ==================== RS485 INTERFACE ================
    
    // ==================== Correct Boards  ================
    //BLUE Client Modbus
    #define RS485_RX_1             GPIO_NUM_6   // RX here maps to RS485 HW-519 module's silk screen "RXD"
    #define RS485_TX_1             GPIO_NUM_7   // TX here maps to RS485 HW-519 module's silk screen "TXD"

    #define RS485_RX_2             GPIO_NUM_15  // RX here maps to RS485 HW-519 module's silk screen "RXD"
    #define RS485_TX_2             GPIO_NUM_16  // TX here maps to RS485 HW-519 module's silk screen "TXD"

    // ==================== RELAY ==========================
    #define RELAY_1_PIN 38  //Pin to toggle the onboard SSR

// Board Version 2 - 2025 
#elif defined(BOARD_VER_V2)
    // analog button array (voltage divider)
    #ifdef CONFIG_IDF_TARGET_ESP32S3
        #define ANALOG_BTN_PIN  A0 //GPIO1
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ANALOG_BTN_PIN  A0
    #else
        #define ANALOG_BTN_PIN  A7
    #endif

    // ==================== SPI DISPLAY ====================
    #define DISPLAY_RST_PIN 46  //Reset
    #define DISPLAY_DC_PIN 3    //Data clock
    #define DISPLAY_CS_PIN 9    //Chip select

    // ==================== RS485 INTERFACE ================
    // ==================== Wrong Boards ================
    //BLUE Client Modbus
    #define RS485_RX_1             GPIO_NUM_15   // RX here maps to RS485 HW-519 module's silk screen "RXD"
    #define RS485_TX_1             GPIO_NUM_16   // TX here maps to RS485 HW-519 module's silk screen "TXD"

    //GREEN Server Modbus
    #define RS485_RX_2             GPIO_NUM_6  // RX here maps to RS485 HW-519 module's silk screen "RXD"
    #define RS485_TX_2             GPIO_NUM_7  // TX here maps to RS485 HW-519 module's silk screen "TXD"

    // ==================== RELAY ==========================
    #define RELAY_1_PIN 38  //Pin to toggle the onboard SSR

#elif defined(BOARD_VER_V3)
    // esp32s3 devkitc n16r8 onboard rgbw led
    #define RGBLED_DATA_PIN 48  

    // analog button array (voltage divider)
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    #define ANALOG_BTN_PIN  A0 //GPIO1
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
    #define ANALOG_BTN_PIN  A0
    #else
    #define ANALOG_BTN_PIN  A7
    #endif

    //SPI OLED display — CircuitSetup `pin_definitions` (ESP32-S3): RST 8, DC 18, CS 17
    // ==================== SPI DISPLAY ====================
    #define DISPLAY_RST_PIN 8   // Reset (was 46; 46 = GPS RX on NESL layout)
    #define DISPLAY_DC_PIN 18   // Data/Command (was 3; 3 = ETH reset on some builds)
    #define DISPLAY_CS_PIN 17   // Chip select

    // ==================== RS485 INTERFACE ================
    // Match CircuitSetup Modbus A / B (MAX485): A = RX 6, TX 4; B = RX 42, TX 7
    #define RS485_RX_1             GPIO_NUM_6   // RS485_RX_A
    #define RS485_TX_1             GPIO_NUM_4   // RS485_TX_A (was 7)
    #define RS485_RX_2             GPIO_NUM_42  // RS485_RX_B (was 15 — now meter CS1_BOARD1)
    #define RS485_TX_2             GPIO_NUM_7   // RS485_TX_B (was 16 — now meter CS2_BOARD1)

    // ==================== RELAY ==========================
    #define RELAY_1_PIN 38  //Pin to toggle the onboard SSR, solid state relay - 5 vdc TTL TBD for larger ssr

    // ==================== I2C 8-ch SSR (PCF8574 module) ===
    // EMS 865B: use the vertical header labeled GND, 5V, SDA, SCL - wire to the SSR
    // module screw terminal (+5V, GND, SCL, SDA) pin-for-pin (SDA->SDA, SCL->SCL).
    // These GPIOs must match how the 865B PCB routes that header to the ESP32-S3.
    // Defaults 4/5 match spare pins on the V001 pinout; if I2C scan finds nothing, ask
    // NESL for the 865B netlist or probe which module pins connect to which ESP pins.
    #define I2C_SSR_SDA_GPIO     14
    #define I2C_SSR_SCL_GPIO     20
    // PCF8574: 0x20-0x27 from DIP A2/A1/A0. PCF8574A often uses 0x38-0x3F.
    #define PCF8574_I2C_ADDR   0x27

    // ==================== CAN INTERFACE ==================
    // SPI shares the meter/display bus. NESL/CircuitSetup uses GPIO2 for pairing LED and
    // GPIO8 for display RST (not SCK); GPIO17 is display CS (not MCP INT). Values below
    // avoid those conflicts — confirm CS/SCK/INT against your 865A/865B schematic.
    #define CAN0_CS     10  // SPI chip select (was 2 — GPIO2 is PAIR_LED / CS2_BOARD2 on NESL)
    #define CAN0_SO     42  // SPI MISO (same net as RS485_RX_B if both used — verify stack)
    #define CAN0_SI     41  // SPI MOSI (same net as MAX485_RE_DE_TOGGLE_B area on some PCBs)
    #define CAN0_SCK    12  // SPI clock (was 8 — GPIO8 is DISPLAY_RST_PIN on NESL)
    #define CAN0_INT    13  // MCP2515 INT (was 17 — GPIO17 is DISPLAY_CS_PIN on NESL)

    // ==================== CIRCUITSETUP 6-CT ENERGY METER ============
    // Shared SPI with CAN + SH1106. Single 6-CT stack: CS1_BOARD1 / CS2_BOARD1 (3-board
    // stacks add CS on 47/2/21/19 per CircuitSetup pin_definitions.h).
    #define CIRCUITSETUP_SPI_MOSI     CAN0_SI
    #define CIRCUITSETUP_SPI_MISO     CAN0_SO
    #define CIRCUITSETUP_SPI_SCK      CAN0_SCK
    #define CIRCUITSETUP_METER_CS_A   15  // CS1_BOARD1 — CT1–CT3 (was 39)
    #define CIRCUITSETUP_METER_CS_B   16  // CS2_BOARD1 — CT4–CT6 (was 40)
    // ATM90 IRQ lines (inputs, diagnostic read only). Not in public pin header — adjust to PCB.
    #define CIRCUITSETUP_IRQ0         33
    #define CIRCUITSETUP_IRQ1         34
#endif

// Fallback defaults for lint-only builds where BOARD_VER_* is undefined.
#ifndef CIRCUITSETUP_SPI_MOSI
#define CIRCUITSETUP_SPI_MOSI 41
#endif
#ifndef CIRCUITSETUP_SPI_MISO
#define CIRCUITSETUP_SPI_MISO 42
#endif
#ifndef CIRCUITSETUP_SPI_SCK
#define CIRCUITSETUP_SPI_SCK 12
#endif
#ifndef CIRCUITSETUP_METER_CS_A
#define CIRCUITSETUP_METER_CS_A 15
#endif
#ifndef CIRCUITSETUP_METER_CS_B
#define CIRCUITSETUP_METER_CS_B 16
#endif
#ifndef CIRCUITSETUP_IRQ0
#define CIRCUITSETUP_IRQ0 33
#endif
#ifndef CIRCUITSETUP_IRQ1
#define CIRCUITSETUP_IRQ1 34
#endif
