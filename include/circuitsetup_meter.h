#pragma once

#include <Arduino.h>

void setup_circuitsetup_meter();
void loop_circuitsetup_meter();

/** JSON for /api/circuitsetup?house=<dashboard meterId 0..n>. Maps house -> CT channel via house % 6. */
String circuitsetup_api_json(int houseMeterId);

float circuitsetup_latest_amps(int ctChannel);
float circuitsetup_phase_voltage(int ctChannel);

/** Copy first MODBUS_NUM_METERS CT channels into global readings[] for MQTT (hack lab). */
void circuitsetup_sync_powerdata_readings();

bool circuitsetup_chip_init_ok(int chipIndex);
