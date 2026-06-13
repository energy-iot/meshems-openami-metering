#pragma once

#ifdef METER_TYPE_ATM90E32

/**
 * @file meter_atm90e32.h
 * @brief Driver for the CircuitSetup Expandable 6-Channel ATM90E32 energy meter.
 *
 * The CircuitSetup 6-channel board carries two ATM90E32 energy-metering ICs.
 * Each IC measures three CT channels (A/B/C) plus one AC voltage reference.
 * Together they provide 6 independent current channels and a shared voltage
 * reading — ideal for sub-circuit (breaker-level) monitoring in a distribution
 * panel.
 *
 * Communication is over SPI; this driver is therefore NOT guarded by
 * ENABLE_MODBUS_MASTER.  Enable it with METER_TYPE_ATM90E32 in platformio.ini.
 *
 * Pin assignments (CS1/CS2 per board, shared MOSI/MISO/CLK) are defined in
 * pins.h.  The SPI bus may be shared with other peripherals (OLED, SD card)
 * provided CS pins are unique.
 *
 * Calibration defaults are taken from the CircuitSetup EmonESP example for
 * meter hardware v1.3+ with a 9 V AC reference transformer.  Override via
 * build flags if different hardware is used:
 *   -DATM90E32_LINE_FREQ=4231        (60 Hz)   or 135 (50 Hz)
 *   -DATM90E32_PGA_GAIN=0            (1×, 21=2×, 42=4×)
 *   -DATM90E32_VOLTAGE_GAIN=7305     (9 V Jameco 157041 — meter v1.3+)
 *   -DATM90E32_CURRENT_GAIN=27961    (100A/50mA SCT-013-000 CT)
 *
 * Readings are stored in the shared readings[] array (data_model.h) with one
 * PowerData entry per channel:
 *   readings[0..2] = IC1 channels A/B/C  (CT1–CT3)
 *   readings[3..5] = IC2 channels A/B/C  (CT4–CT6)
 *
 * Additional boards (add-on pairs) would extend readings[6..11], etc., up to
 * ATM90E32_NUM_BOARDS * 6 channels.  Increase MODBUS_NUM_METERS in
 * data_model.h accordingly.
 *
 * @note Requires the CircuitSetup ATM90E32 Arduino library:
 *       https://github.com/CircuitSetup/ATM90E32_Arduino_Library
 *       Add to platformio.ini lib_deps when METER_TYPE_ATM90E32 is active.
 */

#include <Arduino.h>
#include <ATM90E32.h>
#include "core/data_model.h"
#include "core/pins.h"

// --------------------------------------------------------------------------
// Board count — one "board" = two ATM90E32 ICs = 6 CT channels.
// Override in platformio.ini with -DATM90E32_NUM_BOARDS=N if using add-ons.
// --------------------------------------------------------------------------
#ifndef ATM90E32_NUM_BOARDS
#define ATM90E32_NUM_BOARDS 1
#endif

// Total CT channels across all boards.
#define ATM90E32_NUM_CHANNELS (ATM90E32_NUM_BOARDS * 6)

// --------------------------------------------------------------------------
// Calibration defaults (see file header for override instructions)
// --------------------------------------------------------------------------

// Line frequency register value: 4231 = 60 Hz, 135 = 50 Hz
#ifndef ATM90E32_LINE_FREQ
#define ATM90E32_LINE_FREQ 4231
#endif

// PGA gain: 0 = 1×, 21 = 2×, 42 = 4×
#ifndef ATM90E32_PGA_GAIN
#define ATM90E32_PGA_GAIN 0
#endif

// Voltage gain for 9 V AC transformer (Jameco 157041) on meter v1.3+
#ifndef ATM90E32_VOLTAGE_GAIN
#define ATM90E32_VOLTAGE_GAIN 7305
#endif

// Current gain for 100A/50mA SCT-013-000 CT at PGA 1×.
// Common alternatives (PGA 1×):
// SCT-006 20A/25mA	11143
// SCT-013-030 30A/1V	 8650
// SCT-013-050 50A/1V	 15420
// SCT-010 50A/16.6mA	 41334
// SCT-010 80A/26.6mA	 41660
// SCT-013-000 100A/50mA 27518
// SCT-016 120A/40mA	 41787
// SCT-024 200A/100mA	 27518
// SCT-024 200A/50mA	 55036

#ifndef ATM90E32_CURRENT_GAIN
#define ATM90E32_CURRENT_GAIN 41660
#endif

// --------------------------------------------------------------------------
// Public interface
// --------------------------------------------------------------------------

// Initialise SPI and all ATM90E32 ICs.  Call once from setup_modbus_master().
void setup_atm90e32();

// Poll all ATM90E32 ICs and copy results into readings[].
// Call from poll_energy_meters() inside loop_modbus_master().
void poll_atm90e32();

#endif // METER_TYPE_ATM90E32
