#pragma once

// Modbus infrastructure headers — always included when Modbus is enabled.
#include <metering/modbus_master.h>
#include <metering/modbus_client.h>
#include <metering/modbus_sht20.h>

// Meter-type-specific driver headers.
// Exactly one METER_TYPE_* flag should be set in platformio.ini.
// If none is set, DDS238 is the default (see modbus_master.cpp).
#include <metering/modbus_dds238.h>
#include <metering/modbus_chd130.h>
#include <metering/modbus_ddsu666.h>
// ATM90E32 is SPI-based; included separately via meter_atm90e32.h when needed.
