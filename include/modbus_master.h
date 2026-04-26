#pragma once

#include <ModbusMaster.h>
#include "data_model.h"

/*
Watch out for MAX485 DO VS D1 unloaded SERIAL SIDE AFFECTS - SEE TECH DETAILS AT  https://www.analog.com/en/products/max485.html#part-details
BIAS RESISTORS DETAILED TECH SPECS HERE https://control.com/forums/threads/modbus-standard-termination.20389/
***** must use 620-150 OHM TERMINATION RESISTOR AT A-B FAR END TERMINATIO TO MINIMIZE REFLECTIONS ******

TESTING WITH 150 OHM RESISTOR ACROSS LAST FURTHEST A-B MODBUS RTU ENDPOINT THIS CAN BE 620 OHM ALTERNATIVELY - FURTHER SCOPE TESTING REQUIRED ON CAT5E VS STP RS485 CBLE 
*/

void setup_modbus_clients();
void setup_modbus_master();
void loop_modbus_master();
void update();

// SHT20 temperature/humidity accessors.
// Available to MQTT and other subsystems regardless of energy meter type.
float    get_sht20_temperature();    // degrees Celsius
float    get_sht20_humidity();       // relative humidity, percent (0–100)
uint16_t get_sht20_success_count();  // cumulative successful poll count (0 = no valid data yet)


