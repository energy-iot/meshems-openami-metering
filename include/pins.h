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
#ifdef defined(BOARD_VER_V1)
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
elif defined(BOARD_VER_V2)
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

    //SPI OLED display
    //#define DISPLAY_RST_PIN 2
    //#define DISPLAY_DC_PIN 42
    //#define DISPLAY_CS_PIN 41

    // ==================== SPI DISPLAY ====================
    #define DISPLAY_RST_PIN 46  //Reset
    #define DISPLAY_DC_PIN 3    //Data clock
    #define DISPLAY_CS_PIN 9    //Chip select

    // ==================== RS485 INTERFACE ================
    #define RS485_RX_1             GPIO_NUM_6   // ESP32 RX <- HW-519 TXD
    #define RS485_TX_1             GPIO_NUM_7   // ESP32 TX -> HW-519 RXD
    #define RS485_RX_2             GPIO_NUM_15
    #define RS485_TX_2             GPIO_NUM_16

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
    #define CAN0_CS     2   //SPI chip select
    #define CAN0_SO     42  //SPI MISO
    #define CAN0_SI     41  //SPI MOSI
    #define CAN0_SCK    8   //SPI clock
    #define CAN0_INT    17  //Message interrupt output
#endif
