#ifdef METER_TYPE_ATM90E32

/**
 * @file meter_atm90e32.cpp
 * @brief ATM90E32 6-channel SPI energy meter driver.
 *
 * Manages one or more CircuitSetup 6-channel board pairs.  Each board pair
 * comprises two ATM90E32 ICs (IC1 on CS1, IC2 on CS2) each measuring three
 * CT channels (A/B/C) plus a single AC voltage reference.
 *
 * Readings are placed into the shared readings[] array (data_model.h):
 *   Board 0: readings[0..5]   (IC1 A/B/C → CT1-3, IC2 A/B/C → CT4-6)
 *   Board 1: readings[6..11]  (if ATM90E32_NUM_BOARDS >= 2), etc.
 *
 * setup_atm90e32() must be called once at startup.
 * poll_atm90e32()  must be called periodically (from loop_modbus_master()).
 */

#include <SPI.h>
#include "meter_atm90e32.h"
#include "data_model.h"
#include "pins.h"

// --------------------------------------------------------------------------
// ATM90E32 IC instances — two per board (IC1 = CT1-3, IC2 = CT4-6)
// --------------------------------------------------------------------------
static ATM90E32 ic1[ATM90E32_NUM_BOARDS];
static ATM90E32 ic2[ATM90E32_NUM_BOARDS];

// CS pin arrays — one entry per board, indexed [0..ATM90E32_NUM_BOARDS-1].
// Values come from pins.h; additional board CS pins follow ATM90E32_IC1_CS_1,
// ATM90E32_IC1_CS_2, … if more boards are wired.
static const int cs1[ATM90E32_NUM_BOARDS] = {
    ATM90E32_IC1_CS
#if ATM90E32_NUM_BOARDS >= 2
    , ATM90E32_IC1_CS_1
#endif
#if ATM90E32_NUM_BOARDS >= 3
    , ATM90E32_IC1_CS_2
#endif
};

static const int cs2[ATM90E32_NUM_BOARDS] = {
    ATM90E32_IC2_CS
#if ATM90E32_NUM_BOARDS >= 2
    , ATM90E32_IC2_CS_1
#endif
#if ATM90E32_NUM_BOARDS >= 3
    , ATM90E32_IC2_CS_2
#endif
};

// --------------------------------------------------------------------------
// setup_atm90e32
// --------------------------------------------------------------------------

/**
 * Initialise all ATM90E32 ICs on the SPI bus.
 * Passes the same calibration values (line frequency, PGA gain, voltage gain,
 * current gain) to every IC.  If individual channel calibration is needed,
 * extend this function to accept per-channel current gain arrays.
 *
 * A short delay after each IC begin() allows the ATM90E32 DSP to stabilise
 * before measurements are requested.
 */
void setup_atm90e32() {
    Serial.printf("SETUP: ATM90E32: initialising %d board(s), %d channels\n",
                  ATM90E32_NUM_BOARDS, ATM90E32_NUM_CHANNELS);

    for (int i = 0; i < ATM90E32_NUM_BOARDS; i++) {
        // IC1 handles CT1/CT2/CT3 with voltage reference on Phase A input.
        ic1[i].begin(cs1[i],
                     ATM90E32_LINE_FREQ,
                     ATM90E32_PGA_GAIN,
                     ATM90E32_VOLTAGE_GAIN,
                     ATM90E32_CURRENT_GAIN,
                     ATM90E32_CURRENT_GAIN,
                     ATM90E32_CURRENT_GAIN);

        // IC2 handles CT4/CT5/CT6 with a second voltage reference on Phase A.
        ic2[i].begin(cs2[i],
                     ATM90E32_LINE_FREQ,
                     ATM90E32_PGA_GAIN,
                     ATM90E32_VOLTAGE_GAIN,
                     ATM90E32_CURRENT_GAIN,
                     ATM90E32_CURRENT_GAIN,
                     ATM90E32_CURRENT_GAIN);

        delay(200); // allow ATM90E32 DSP to settle
    }

    Serial.println("SETUP: ATM90E32: ready");
}

// --------------------------------------------------------------------------
// poll_atm90e32
// --------------------------------------------------------------------------

/**
 * Poll all ATM90E32 ICs and populate readings[] with fresh measurements.
 *
 * For each board:
 *   - Reads the IC1 system status to check for a live SPI connection.
 *     If the status word is 0x0000 or 0xFFFF the IC is not responding and
 *     that board is skipped (its readings[] entries are left unchanged).
 *   - Reads voltage, current, active power, apparent power, and power factor
 *     for each of the six CT channels.
 *   - Reads frequency and IC temperature once from IC1 per board and copies
 *     them into all six channel entries for that board.
 *
 * Sign of real power: the ATM90E32 current registers are unsigned, so a
 * negative active power value means the CT is on an export (generation)
 * circuit; the current reading is flipped to match.
 */
void poll_atm90e32() {
    for (int board = 0; board < ATM90E32_NUM_BOARDS; board++) {
        // Validate SPI comms — 0x0000 or 0xFFFF indicates no response.
        unsigned short sys0 = ic1[board].GetSysStatus0();
        if (sys0 == 0x0000 || sys0 == 0xFFFF) {
            #ifdef ENABLE_DEBUG
            Serial.printf("ATM90E32 board %d: no response (SysStatus0=0x%04X), skipping\n",
                          board, sys0);
            #endif
            continue;
        }

        // Per-IC readings — indexed as [0..5] within this board.
        float voltage[6];
        float current[6];
        float active_power[6];
        float apparent_power[6];
        float power_factor[6];

        // IC1 — channels 0/1/2 (CT1/CT2/CT3 on this board)
        voltage[0]       = ic1[board].GetLineVoltageA();
        voltage[1]       = ic1[board].GetLineVoltageB();
        voltage[2]       = ic1[board].GetLineVoltageC();
        current[0]       = ic1[board].GetLineCurrentA();
        current[1]       = ic1[board].GetLineCurrentB();
        current[2]       = ic1[board].GetLineCurrentC();
        active_power[0]  = ic1[board].GetActivePowerA();
        active_power[1]  = ic1[board].GetActivePowerB();
        active_power[2]  = ic1[board].GetActivePowerC();
        apparent_power[0] = ic1[board].GetApparentPowerA();
        apparent_power[1] = ic1[board].GetApparentPowerB();
        apparent_power[2] = ic1[board].GetApparentPowerC();
        power_factor[0]  = ic1[board].GetPowerFactorA();
        power_factor[1]  = ic1[board].GetPowerFactorB();
        power_factor[2]  = ic1[board].GetPowerFactorC();

        // IC2 — channels 3/4/5 (CT4/CT5/CT6 on this board)
        voltage[3]       = ic2[board].GetLineVoltageA();
        voltage[4]       = ic2[board].GetLineVoltageB();
        voltage[5]       = ic2[board].GetLineVoltageC();
        current[3]       = ic2[board].GetLineCurrentA();
        current[4]       = ic2[board].GetLineCurrentB();
        current[5]       = ic2[board].GetLineCurrentC();
        active_power[3]  = ic2[board].GetActivePowerA();
        active_power[4]  = ic2[board].GetActivePowerB();
        active_power[5]  = ic2[board].GetActivePowerC();
        apparent_power[3] = ic2[board].GetApparentPowerA();
        apparent_power[4] = ic2[board].GetApparentPowerB();
        apparent_power[5] = ic2[board].GetApparentPowerC();
        power_factor[3]  = ic2[board].GetPowerFactorA();
        power_factor[4]  = ic2[board].GetPowerFactorB();
        power_factor[5]  = ic2[board].GetPowerFactorC();

        // Frequency and temperature are global per IC1 measurement.
        float freq = ic1[board].GetFrequency();
        float temp = ic1[board].GetTemperature();

        // Energy accumulators — per-channel absolute Wh since last IC reset.
        // GetImportEnergyA/B/C and GetExportEnergyA/B/C return Wh; divide by
        // 1000 to store in the data model's kWh convention.
        // TODO: add a software interval-energy accumulator (delta kWh between
        // polls) for 15-min / hourly / daily billing intervals.
        // kWh values — double→float truncation done after dividing by 1000.
        float import_energy_kwh[6];
        float export_energy_kwh[6];
        // GetImportEnergy*/GetExportEnergy* return double (Wh).
        // Divide in double arithmetic before truncating to float to preserve
        // sub-Wh resolution up to at least ~16 GWh (double mantissa limit).
        import_energy_kwh[0] = (float)(ic1[board].GetImportEnergyA() / 1000.0);
        import_energy_kwh[1] = (float)(ic1[board].GetImportEnergyB() / 1000.0);
        import_energy_kwh[2] = (float)(ic1[board].GetImportEnergyC() / 1000.0);
        import_energy_kwh[3] = (float)(ic2[board].GetImportEnergyA() / 1000.0);
        import_energy_kwh[4] = (float)(ic2[board].GetImportEnergyB() / 1000.0);
        import_energy_kwh[5] = (float)(ic2[board].GetImportEnergyC() / 1000.0);
        export_energy_kwh[0] = (float)(ic1[board].GetExportEnergyA() / 1000.0);
        export_energy_kwh[1] = (float)(ic1[board].GetExportEnergyB() / 1000.0);
        export_energy_kwh[2] = (float)(ic1[board].GetExportEnergyC() / 1000.0);
        export_energy_kwh[3] = (float)(ic2[board].GetExportEnergyA() / 1000.0);
        export_energy_kwh[4] = (float)(ic2[board].GetExportEnergyB() / 1000.0);
        export_energy_kwh[5] = (float)(ic2[board].GetExportEnergyC() / 1000.0);

        // Populate shared readings[] — base index for this board is board*6.
        int base = board * 6;
        unsigned long now_ms = millis();
        for (int ch = 0; ch < 6; ch++) {
            // Flip current sign on export circuits (negative active power).
            if (active_power[ch] < 0.0f) {
                current[ch] = -current[ch];
            }

            readings[base + ch].voltage       = voltage[ch];
            readings[base + ch].current       = current[ch];
            // ATM90E32 active power is in watts; convert to kW for data model.
            readings[base + ch].active_power  = active_power[ch] / 1000.0f;
            // ATM90E32 does not natively provide reactive power; zero the field.
            // TODO: derive reactive power from apparent and active if needed.
            readings[base + ch].reactive_power = 0.0f;
            // ATM90E32 apparent power is in VA; convert to kVA for data model.
            readings[base + ch].apparent_power = apparent_power[ch] / 1000.0f;
            readings[base + ch].power_factor   = power_factor[ch];
            readings[base + ch].frequency      = freq;
            // Energy already in kWh (division done in double above).
            readings[base + ch].import_energy  = import_energy_kwh[ch];
            readings[base + ch].export_energy  = export_energy_kwh[ch];
            readings[base + ch].total_energy   =
                readings[base + ch].import_energy + readings[base + ch].export_energy;
            // Assign CT channel index as meter ID and record poll timestamp.
            readings[base + ch].meterid              = (uint8_t)(base + ch);
            readings[base + ch].timestamp_last_report = now_ms;
        }

        #ifdef ENABLE_DEBUG
        Serial.printf("ATM90E32 board %d: V1=%.1fV freq=%.2fHz temp=%.1fC\n",
                      board, voltage[0], freq, temp);
        for (int ch = 0; ch < 6; ch++) {
            Serial.printf("  CT%d: %.3fA  %.2fW  PF=%.3f\n",
                          base + ch + 1,
                          readings[base + ch].current,
                          readings[base + ch].active_power * 1000.0f,
                          readings[base + ch].power_factor);
        }
        #endif
    }
}

#endif // METER_TYPE_ATM90E32
