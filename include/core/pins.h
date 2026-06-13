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
 * - SD card interface (W5500 Ethernet and SD card share SPI bus; separate CS pins)
 * - Ethernet controller (W5500)
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

    // ==================== CAN INTERFACE (MCP2515) ==================
    #define CAN0_CS     2   //SPI chip select
    #define CAN0_SO     42  //SPI MISO
    #define CAN0_SI     41  //SPI MOSI
    #define CAN0_SCK    8   //SPI clock
    #define CAN0_INT    17  //Message interrupt output

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

    // ==================== CAN INTERFACE (MCP2515) ==================
    #define CAN0_CS     2   //SPI chip select
    #define CAN0_SO     42  //SPI MISO
    #define CAN0_SI     41  //SPI MOSI
    #define CAN0_SCK    8   //SPI clock
    #define CAN0_INT    17  //Message interrupt output

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

    // ==================== SD CARD ====================
    // Verified by Liam April 23 2026
    #define SD_CS GPIO_NUM_10
    #define SD_CLK GPIO_NUM_12
    #define SD_MOSI GPIO_NUM_11
    #define SD_MISO GPIO_NUM_13

    // ==================== Ethernet Interface ====================
    // Verified by Liam May 1 2026
    #define ETH_CS GPIO_NUM_9
    #define ETH_CLK GPIO_NUM_12
    #define ETH_MOSI GPIO_NUM_11
    #define ETH_MISO GPIO_NUM_13
    #define ETH_RST GPIO_NUM_3

    // ==================== SPI OLED DISPLAY ====================
    // Verified by Liam April 23 2026
    #define DISPLAY_RST_PIN  GPIO_NUM_8     // RST
    #define DISPLAY_CLK_PIN GPIO_NUM_12     // CLK
    #define DISPLAY_MOSI_PIN GPIO_NUM_11    // MOSI
    #define DISPLAY_DC_PIN GPIO_NUM_18      // DC
    #define DISPLAY_CS_PIN GPIO_NUM_17      // CS

    // ==================== RS485 INTERFACE ================

    // ==================== NESL EMS Controller PCB 865B Pinout ==========================
    // Pin 42 = HW-RXD "Modbus 1 - Master" (connects to "Temp Senser" modbus)
    // Pin 7 = HW-TXD "Modbus 1 - Master" (connects to "Temp Senser" modbus)

    // Pin 6 = HW-RXD "Modbus 2 - Client" (second modbus SD - card side)
    // Pin 4 = HW-TXD "Modbus 2 - Client" (second modbus SD - card side)

    // -----------------    RS485_1 INTERFACE   -----------------------
    #define RS485_1_RX  GPIO_NUM_42  // Connects to HW-519 RXD
    #define RS485_1_TX  GPIO_NUM_7   // Connects to HW-519 TXD

    // -----------------    RS485_2 INTERFACE   -----------------------
    #define RS485_2_RX  GPIO_NUM_6 // Connects to HW-519 RXD
    #define RS485_2_TX  GPIO_NUM_4 // Connects to HW-519 RXD

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

    // ==================== ATM90E32 6-CHANNEL SPI METER ====================
    // The ATM90E32 shares the same SPI bus as the OLED/SD (MOSI=11, CLK=12,
    // MISO=13) but requires unique chip-select pins per IC.

    // For a single-board setup (ATM90E32_NUM_BOARDS=1), only IC1_CS and IC2_CS are used.
    // For add-on boards (ATM90E32_NUM_BOARDS >= 2), define additional CS pins:
    //   ATM90E32_IC1_CS_1, ATM90E32_IC2_CS_1   (board 2)
    //   ATM90E32_IC1_CS_2, ATM90E32_IC2_CS_2   (board 3)
    // etc., and increase ATM90E32_NUM_BOARDS in platformio.ini accordingly.
    #define ATM90E32_MOSI        GPIO_NUM_11  // Shared SPI bus (OLED/SD)
    #define ATM90E32_MISO        GPIO_NUM_13  // Shared SPI bus (OLED/SD)
    #define ATM90E32_CLK         GPIO_NUM_12  // Shared SPI bus (OLED/SD)
    #define ATM90E32_IC1_CS      GPIO_NUM_15  // Verifed by Liam May 1 2026
    #define ATM90E32_IC2_CS      GPIO_NUM_18  // Verifed by Liam May 1 2026

    // Aliases used by circuitsetup_meter.cpp — map to verified ATM90E32 SPI bus pins above
    #define CIRCUITSETUP_SPI_MOSI     ATM90E32_MOSI
    #define CIRCUITSETUP_SPI_MISO     ATM90E32_MISO
    #define CIRCUITSETUP_SPI_SCK      ATM90E32_CLK
    #define CIRCUITSETUP_METER_CS_A   ATM90E32_IC1_CS
    #define CIRCUITSETUP_METER_CS_B   ATM90E32_IC2_CS
    #define CIRCUITSETUP_IRQ0         33
    #define CIRCUITSETUP_IRQ1         34
#endif

