#include <metering/modbus_ddsu666.h>
#include <TimeLib.h>
#include <core/data_model.h>

/**
 * @file modbus_ddsu666.cpp
 * @brief Modbus RTU driver for the Chint DDSU666 single-phase energy meter.
 *
 * Register map and scaling are based on the Chint DDSU666 Modbus protocol
 * specification.  All register addresses and scale factors should be verified
 * against your specific meter firmware before production deployment.
 *
 * Active power (rACTIVE_POWER) is a signed 32-bit value; negative means
 * the circuit is exporting (generation).  reactive power is also signed.
 * Current (rCURRENT) is unsigned — its sign is inferred from active power.
 */

Modbus_DDSU666::Modbus_DDSU666() {}

uint8_t Modbus_DDSU666::get_modbus_address() {
    return modbus_address;
}

void Modbus_DDSU666::set_modbus_address(uint8_t addr) {
    modbus_address = addr;
}

// Read a single 16-bit holding register and return the raw uint16 as float.
float Modbus_DDSU666::read_modbus_value(uint16_t registerAddress) {
    uint8_t result = readHoldingRegisters(registerAddress, 1);
    if (result != ku8MBSuccess) {
        Serial.printf("MODBUS DDSU666: Error reading register 0x%04X\n", registerAddress);
        throw std::runtime_error("Modbus read error");
    }
    return (float)getResponseBuffer(0);
}

// Read two consecutive 16-bit holding registers and return the combined
// 32-bit value as float (high word first).
float Modbus_DDSU666::read_modbus_extended_value(uint16_t registerAddress) {
    uint8_t result = readHoldingRegisters(registerAddress, 2);
    if (result != ku8MBSuccess) {
        Serial.printf("MODBUS DDSU666: Error reading register 0x%04X\n", registerAddress);
        throw std::runtime_error("Modbus read error");
    }
    return (float)((getResponseBuffer(0) << 16) | getResponseBuffer(1));
}

// Read a signed 32-bit holding register pair (big-endian, high word first).
static float read_signed32(float raw) {
    // Reinterpret unsigned 32-bit bit pattern as signed int32 then cast.
    int32_t s = (int32_t)(uint32_t)raw;
    return (float)s;
}

void Modbus_DDSU666::poll() {
    try {
        // Voltage: 0.1 V/LSB (16-bit, unsigned)
        last_reading.voltage       = read_modbus_value(rVOLTAGE) / 10.0f;

        // Current: 0.001 A/LSB (32-bit, unsigned)
        last_reading.current       = read_modbus_extended_value(rCURRENT) / 1000.0f;

        // Active Power: 0.1 W/LSB (32-bit, signed) — store in kW
        last_reading.active_power  =
            read_signed32(read_modbus_extended_value(rACTIVE_POWER)) / 10000.0f;

        // Reactive Power: 0.1 VAr/LSB (32-bit, signed) — store in kVAr
        last_reading.reactive_power =
            read_signed32(read_modbus_extended_value(rREACTIVE_POWER)) / 10000.0f;

        // Power Factor: 0.001/LSB (16-bit, signed)
        last_reading.power_factor  = read_modbus_value(rPOWER_FACTOR) / 1000.0f;

        // Frequency: 0.01 Hz/LSB (16-bit, unsigned)
        last_reading.frequency     = read_modbus_value(rFREQUENCY) / 100.0f;

        // Import Energy: 0.01 kWh/LSB (32-bit, unsigned)
        last_reading.import_energy = read_modbus_extended_value(rIMPORT_ENERGY) / 100.0f;

        // Export Energy: 0.01 kWh/LSB (32-bit, unsigned)
        last_reading.export_energy = read_modbus_extended_value(rEXPORT_ENERGY) / 100.0f;

        // Total energy = import + export (DDSU666 has no dedicated total register)
        last_reading.total_energy  = last_reading.import_energy + last_reading.export_energy;

        last_reading.timestamp_last_report = now();

        #ifdef ENABLE_DEBUG
        Serial.printf("MODBUS DDSU666: V=%.1fV I=%.3fA P=%.3fkW Q=%.3fkVAr PF=%.3f F=%.2fHz\n",
                      last_reading.voltage, last_reading.current,
                      last_reading.active_power, last_reading.reactive_power,
                      last_reading.power_factor, last_reading.frequency);
        Serial.printf("MODBUS DDSU666: Import=%.2fkWh Export=%.2fkWh\n",
                      last_reading.import_energy, last_reading.export_energy);
        #endif

    } catch (std::runtime_error& e) {
        Serial.println("MODBUS DDSU666: Error reading registers");
    }
}

// --------------------------------------------------------------------------
// Getters — return last cached value; call poll() first.
// --------------------------------------------------------------------------
float Modbus_DDSU666::getTotalEnergy()    { return last_reading.total_energy; }
float Modbus_DDSU666::getImportEnergy()   { return last_reading.import_energy; }
float Modbus_DDSU666::getExportEnergy()   { return last_reading.export_energy; }
float Modbus_DDSU666::getVoltage()        { return last_reading.voltage; }
float Modbus_DDSU666::getCurrent()        { return last_reading.current; }
float Modbus_DDSU666::getActivePower()    { return last_reading.active_power; }
float Modbus_DDSU666::getReactivePower()  { return last_reading.reactive_power; }
float Modbus_DDSU666::getPowerFactor()    { return last_reading.power_factor; }
float Modbus_DDSU666::getFrequency()      { return last_reading.frequency; }
