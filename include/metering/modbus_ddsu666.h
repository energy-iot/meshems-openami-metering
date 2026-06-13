#pragma once

#include <metering/modbus_master.h>
#include "core/data_model.h"

/**
 * @file modbus_ddsu666.h
 * @brief Modbus RTU driver class for the Chint DDSU666 single-phase energy meter.
 *
 * The DDSU666 is a single-phase DIN-rail energy meter with RS-485 Modbus RTU
 * output, commonly used for tenant-level sub-metering in distribution panels.
 * It provides voltage, current, active/reactive power, power factor,
 * frequency, and cumulative energy (import/export).
 *
 * Register map (function code 0x03 — Read Holding Registers):
 *   All addresses below are as documented in the Chint DDSU666 Modbus protocol
 *   specification.  Verify against your specific firmware revision if readings
 *   appear incorrect.
 *
 *   0x2000  Voltage         (0.1 V per LSB,    16-bit unsigned)
 *   0x2002  Current         (0.001 A per LSB,  32-bit unsigned)
 *   0x2004  Active Power    (0.1 W per LSB,    32-bit signed — negative = export)
 *   0x200A  Reactive Power  (0.1 VAr per LSB,  32-bit signed)
 *   0x2010  Power Factor    (0.001 per LSB,    16-bit signed)
 *   0x2014  Frequency       (0.01 Hz per LSB,  16-bit unsigned)
 *   0x4000  Import Energy   (0.01 kWh per LSB, 32-bit unsigned)
 *   0x400A  Export Energy   (0.01 kWh per LSB, 32-bit unsigned)
 *
 * Default serial settings: 9600 8N1.
 * Default Modbus node address: 0x01 (configurable via front panel).
 *
 * TODO: Confirm full register map and scaling factors from official Chint
 *       DDSU666 Modbus protocol documentation before production deployment.
 *
 * AUTHORS: Glenn Algie
 */

class Modbus_DDSU666 : public ModbusMaster {
    public:
        Modbus_DDSU666();
        ~Modbus_DDSU666() {};

        uint8_t get_modbus_address();
        void set_modbus_address(uint8_t addr);

        float read_modbus_value(uint16_t registerAddress);
        float read_modbus_extended_value(uint16_t registerAddress);

        enum MB_Reg {
            rVOLTAGE        = 0x2000,  // 0.1 V/LSB,   16-bit unsigned
            rCURRENT        = 0x2002,  // 0.001 A/LSB,  32-bit unsigned
            rACTIVE_POWER   = 0x2004,  // 0.1 W/LSB,    32-bit signed
            rREACTIVE_POWER = 0x200A,  // 0.1 VAr/LSB,  32-bit signed
            rPOWER_FACTOR   = 0x2010,  // 0.001/LSB,    16-bit signed
            rFREQUENCY      = 0x2014,  // 0.01 Hz/LSB,  16-bit unsigned
            rIMPORT_ENERGY  = 0x4000,  // 0.01 kWh/LSB, 32-bit unsigned
            rEXPORT_ENERGY  = 0x400A   // 0.01 kWh/LSB, 32-bit unsigned
        };

        void poll();
        PowerData last_reading;

        float getTotalEnergy();
        float getImportEnergy();
        float getExportEnergy();
        float getVoltage();
        float getCurrent();
        float getActivePower();
        float getReactivePower();
        float getPowerFactor();
        float getFrequency();

    private:
        uint8_t modbus_address;
        unsigned long timestamp_last_report;
        unsigned long timestamp_last_failure;
};

extern Modbus_DDSU666 ddsu666_1;
extern Modbus_DDSU666 ddsu666_2;
extern Modbus_DDSU666 ddsu666_3;
