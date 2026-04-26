#pragma once

#include <stdint.h>

// Number of meter slots in readings[].
// ATM90E32 provides 6 independent CT channels per board pair; all other
// Modbus meters use 3 slots (one per physical unit).
// Increase ATM90E32_NUM_BOARDS in platformio.ini for multi-board setups.
#if defined(METER_TYPE_ATM90E32)
  #ifndef ATM90E32_NUM_BOARDS
    #define ATM90E32_NUM_BOARDS 1
  #endif
  #define MODBUS_NUM_METERS (ATM90E32_NUM_BOARDS * 6)
#else
  #define MODBUS_NUM_METERS 3
#endif
#define MODBUS_NUM_THERMOSTATS 1

#define MODBUS_NUM_COILS                2
#define MODBUS_NUM_DISCRETE_INPUTS      2
#define MODBUS_NUM_HOLDING_REGISTERS    2
#define MODBUS_NUM_INPUT_REGISTERS      4
#define CURRENT_HISTORY_SIZE            128  // Store 128 historical readings

extern bool coils[MODBUS_NUM_COILS];
extern bool discreteInputs[MODBUS_NUM_DISCRETE_INPUTS];
extern uint16_t holdingRegisters[MODBUS_NUM_HOLDING_REGISTERS];
extern uint16_t inputRegisters[MODBUS_NUM_INPUT_REGISTERS];

 // Struct for current, voltage, and power factor
//EMS as a 3 phase subpanel with N  meters and x meters per phase SO has multiple dimensions of powerdata to totalize and publish
// 1. all 3 phases totalized powerdata usage per StreetPoleSubpanel
// 2. each Phase powerdata summary per subpanel
// 3. each singlephase meter powerdata including which phase - TODO o\is qr code all subpanel networked items
// powerdata has leakage and harmonic transients that are key to track for periodic engineering Operations and maintenance and rebalancing alerts
// for a future staging/installer app, all networking device shall have meaningful QR codes
// Staging or install or at maintenance time  scan the subpanel and all active networking parts installed to the subpanel
// are auto provisoned in a backend Db addressable to the MS subpanel globally unique QR code

// Unit convention for PowerData power fields:
//   active_power   — kW   (kilowatts)
//   reactive_power — kVAr (kilovolt-amperes reactive)
//   apparent_power — kVA  (kilovolt-amperes apparent)
//   energy fields  — kWh  (kilowatt-hours)
// MQTT serialisers multiply by 1000 to produce W / VAr / VA / Wh for SunSpec compliance.
struct PowerData {  // single phase per meter data, equivalent to per tenant
    unsigned long timestamp_last_report = 0;
    float total_energy = 0;    // kWh
    float export_energy = 0;   // kWh
    float import_energy = 0;   // kWh
    float stored_energy = 0;   // kWh
    float transform_energy = 0; // tracks total energy transformed AC-DC inverted and converted dc-dc or ac-ac
    float voltage = 0;         // V
    float current = 0;         // A
    float active_power = 0;    // kW
    float reactive_power = 0;  // kVAr
    float apparent_power = 0;  // kVA
    float power_factor = 0;    // 0-1
    float frequency = 0;       // Hz
    int8_t phase = 0;          // 0=A, 1=B, 2=C (phase assignment in 3-phase subpanel)
    uint8_t meterid = 0;       // Modbus node number or CT channel index (0-based)
    float metadata = 0;        // 1-247 (high byte), 1-16 (low byte)
};

struct Power3PhData {  // each EMS subpanel has 3phase multiple tenants  per pahse multiple per phase totals
    unsigned long timestamp_last_report = 0;
    //TODO add 3PhasePowerData cached data items here
    float metadata = 0;      // 1-247 (high byte), 1-16 (low byte)
};

struct LeakageData {  // TODO expand this struct for all powerdata OR have different structs
    unsigned long timestamp_last_report = 0;
    //TODO add LeakageData cached data items here
    float metadata = 0;      // 1-247 (high byte), 1-16 (low byte)
};
struct HarmonicsData {  // TODO expand this struct for all powerdata OR have different structs
    unsigned long timestamp_last_report = 0;
    //TODO add Harmonics cached data items here
    float metadata = 0;      // 1-247 (high byte), 1-16 (low byte)
};

// Current history data structure for timeline graph
typedef struct {
    float values[CURRENT_HISTORY_SIZE];  // Circular buffer for current values
    int currentIndex;                    // Current position in the buffer
    int count;                           // Number of readings stored (up to CURRENT_HISTORY_SIZE)
    float minValue;                      // Minimum value in the buffer (for auto-scaling)
    float maxValue;                      // Maximum value in the buffer (for auto-scaling)
} CurrentHistory;

extern CurrentHistory currentHistory;
extern PowerData readings[]; // Array to hold readings for each meter

// Function to add a new current reading to the history buffer
void addCurrentReading(float value);

extern PowerData last_reading; // older cache data to allow easier iterative design testing and debugging
extern Power3PhData last_EMS_power_reading;  // 3 phase streetpole EMS ( include EMS subpanel scoped energy data totalized per phase)
extern HarmonicsData last_harmonics_reading;    // TODO breakout Harmonics data Current and Voltage per phase as its own 
extern LeakageData last_leakage_reading;        // Leakage data is measured reported and actionable  perm streetPoleEMS per phase
                                                // TODO single phase meter can have per phase leakage either as mRCM or RCD
                                                // RCD leakage will set fault and cause tenant contactor to open circuit 
                                                // RCM leakage will report per tenant leakage measurements and autonoomous trigger of tenant contactor to open if hits life threatening levels as per Type B
                                                // if per phase leakage is measured then may not have to per tenant leakage - downside is the\\at all tenants on a phase get opencircuited if phase leakage RCM hits life threatening levels

