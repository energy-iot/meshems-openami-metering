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

    // ==================== SD CARD ====================
    // Verified by Liam April 23 2026 
    #define SD_MOSI 11
    #define SD_CLK 12    
    #define SD_MISO 13

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

    // ==================== CAN INTERFACE (MCP2515) ==================
    // ==== NOTE: There is no MCP 2515 on the 865B PCB pins are defined to prevent compilation error ===

    // Uses a separate SPIClass (canSPI) — does NOT share pins with the OLED SPI bus
    // (OLED occupies GPIO 11/12/13 via the default SPI instance).
    #define CAN0_CS     10  // SPI chip select
    #define CAN0_SO     15  // SPI MISO
    #define CAN0_SI     16  // SPI MOSI
    #define CAN0_SCK    17  // SPI clock
    #define CAN0_INT    18  // MCP2515 interrupt output (active-low)
#endif

