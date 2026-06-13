#ifdef ENABLE_MODBUS_MASTER
/**
 * @file modbus_master.cpp
 * @brief Energy meter master subsystem — polls all attached meter hardware.
 *
 * This file is the single entry-point for meter setup and polling regardless
 * of which meter type is selected.  The active meter back-end is chosen at
 * compile time by exactly one of the METER_TYPE_* build flags:
 *
 *   METER_TYPE_DDS238   — DDS238 single-phase Modbus RTU (default)
 *   METER_TYPE_CHD130   — CHINT CHD130 single-phase Modbus RTU
 *   METER_TYPE_DDSU666  — CHINT DDSU666 single-phase Modbus RTU
 *   METER_TYPE_ATM90E32 — CircuitSetup 6-channel ATM90E32 SPI meter
 *
 * If no METER_TYPE_* flag is defined, METER_TYPE_DDS238 is used as the
 * default to preserve backward compatibility.
 *
 * RS-485 bus and SHT20 thermostat are always initialised regardless of
 * meter type.  When METER_TYPE_ATM90E32 is active, energy metering uses SPI
 * while the SHT20 temperature/humidity sensor continues to use RS-485 — the
 * two buses are fully independent.
 *
 * Hardware: HW519 RS-485 transceiver on RS485_1_RX / RS485_1_TX (pins.h).
 */

#include <metering/modbus.h>
#include <core/pins.h>
#include <core/data_model.h>
#include <core/config.h>
#include <math.h>

// --------------------------------------------------------------------------
// Default meter type — keeps existing deployments working unchanged.
// --------------------------------------------------------------------------
#if !defined(METER_TYPE_DDS238)  && \
    !defined(METER_TYPE_CHD130)  && \
    !defined(METER_TYPE_DDSU666) && \
    !defined(METER_TYPE_ATM90E32)
#define METER_TYPE_DDS238
#endif

// --------------------------------------------------------------------------
// Meter-type-specific includes
// --------------------------------------------------------------------------

// SoftwareSerial and SHT20 are always needed: the RS-485 bus hosts the SHT20
// temperature/humidity sensor regardless of the energy meter type.
#include <SoftwareSerial.h>
#include <metering/modbus_sht20.h>

#if defined(METER_TYPE_ATM90E32)
    #include <metering/meter_atm90e32.h>
#elif defined(METER_TYPE_DDS238)
    #include <metering/modbus_dds238.h>
#elif defined(METER_TYPE_CHD130)
    #include <metering/modbus_chd130.h>
#elif defined(METER_TYPE_DDSU666)
    #include <metering/modbus_ddsu666.h>
#endif

// --------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------

// RS-485 baud rate for all Modbus RTU meters; DDS238, CHD130, and DDSU666
// all default to 9600 8N1 from the factory.
#define RS485_1_BAUD 9600

#define THERMOSTAT_1_ADDR 0x01

// Modbus node addresses.  Standard factory default for DDS238, CHD130, and
// DDSU666 meters is 0x01/0x02/0x03.  Reprogramme each meter via its front
// panel or configuration tool before installation if a different address is
// required.
#define METER_1_ADDR 0x01
#define METER_2_ADDR 0x02
#define METER_3_ADDR 0x03

// --------------------------------------------------------------------------
// RS-485 bus objects — always present; shared by SHT20 and Modbus meters.
// --------------------------------------------------------------------------
SoftwareSerial _modbus1(RS485_1_RX, RS485_1_TX); // HW519 RS-485 transceiver
Modbus_SHT20   sht20;                             // temperature/humidity sensor

// Modbus energy meter objects — only for RTU meter types.
#if defined(METER_TYPE_DDS238)
Modbus_DDS238  dds238_1;
Modbus_DDS238  dds238_2;
Modbus_DDS238  dds238_3;

#elif defined(METER_TYPE_CHD130)
Modbus_CHD130  chd130_1;
Modbus_CHD130  chd130_2;
Modbus_CHD130  chd130_3;

#elif defined(METER_TYPE_DDSU666)
Modbus_DDSU666 ddsu666_1;
Modbus_DDSU666 ddsu666_2;
Modbus_DDSU666 ddsu666_3;
#endif

// Poll timestamp for loop_modbus_master() rate limiting.
static unsigned long lastPollMillis = 0;

// Reserved for a future EVSE charge-session state machine; not yet used.
static unsigned long lastEVSEMillis         = 0;
static unsigned long lastEVSEChargingMillis = 0;

// --------------------------------------------------------------------------
// SHT20 thermostat setup — always present regardless of energy meter type
// --------------------------------------------------------------------------
// Initialise the SHT20 temperature/humidity sensor at THERMOSTAT_1_ADDR.
// The SHT20 sits on the RS-485 bus that is separate from the ATM90E32 SPI bus.
static void setup_sht20() {
    Serial.printf("SETUP: MODBUS: SHT20 #1: address:0x%02X\n", THERMOSTAT_1_ADDR);
    sht20.set_modbus_address(THERMOSTAT_1_ADDR);
    sht20.begin(THERMOSTAT_1_ADDR, _modbus1);
}

// --------------------------------------------------------------------------
// Per-meter-type setup helpers
// --------------------------------------------------------------------------
#if defined(METER_TYPE_DDS238)
// Initialise three DDS238 meters at their factory-staged Modbus node addresses.
static void setup_meters() {
    Serial.printf("SETUP: MODBUS: DDS238 meters: 0x%02X, 0x%02X, 0x%02X\n",
                  METER_1_ADDR, METER_2_ADDR, METER_3_ADDR);
    dds238_1.begin(METER_1_ADDR, _modbus1);
    dds238_2.begin(METER_2_ADDR, _modbus1);
    dds238_3.begin(METER_3_ADDR, _modbus1);
}

#elif defined(METER_TYPE_CHD130)
// Initialise three CHD130 meters at their factory-staged Modbus node addresses.
static void setup_meters() {
    Serial.printf("SETUP: MODBUS: CHD130 meters: 0x%02X, 0x%02X, 0x%02X\n",
                  METER_1_ADDR, METER_2_ADDR, METER_3_ADDR);
    chd130_1.begin(METER_1_ADDR, _modbus1);
    chd130_2.begin(METER_2_ADDR, _modbus1);
    chd130_3.begin(METER_3_ADDR, _modbus1);
}

#elif defined(METER_TYPE_DDSU666)
// Initialise three DDSU666 meters at their factory-staged Modbus node addresses.
static void setup_meters() {
    Serial.printf("SETUP: MODBUS: DDSU666 meters: 0x%02X, 0x%02X, 0x%02X\n",
                  METER_1_ADDR, METER_2_ADDR, METER_3_ADDR);
    ddsu666_1.begin(METER_1_ADDR, _modbus1);
    ddsu666_2.begin(METER_2_ADDR, _modbus1);
    ddsu666_3.begin(METER_3_ADDR, _modbus1);
}
#endif

// --------------------------------------------------------------------------
// setup_modbus_clients — called by setup_modbus_master after bus init
// --------------------------------------------------------------------------
void setup_modbus_clients() {
    // SHT20 is always initialised — it lives on RS-485 independent of the
    // energy meter type.
    setup_sht20();

#if defined(METER_TYPE_ATM90E32)
    // SPI energy meter: initialise ATM90E32 ICs in addition to SHT20.
    setup_atm90e32();
#else
    // Modbus RTU energy meters share the same RS-485 bus as the SHT20.
    setup_meters();
#endif
}

// --------------------------------------------------------------------------
// setup_modbus_master
// --------------------------------------------------------------------------

/**
 * Initialise the energy meter subsystem.
 *
 * Always resets RS-485 GPIO mux assignments and opens SoftwareSerial so the
 * SHT20 sensor is reachable on all meter-type configurations.  For Modbus
 * energy meters the same bus also carries the meter devices.  For ATM90E32,
 * SPI initialisation is handled inside setup_atm90e32().
 */
void setup_modbus_master() {
    // RS-485 bus is always needed: the SHT20 sensor uses it regardless of
    // which energy meter back-end is selected.
    gpio_reset_pin(RS485_1_RX);
    gpio_reset_pin(RS485_1_TX);
    gpio_reset_pin(RS485_2_RX);
    gpio_reset_pin(RS485_2_TX);
    _modbus1.begin(RS485_1_BAUD);

    setup_modbus_clients();
}

// --------------------------------------------------------------------------
// Data model update — copy cached meter readings into readings[]
// --------------------------------------------------------------------------

/**
 * Copy the latest meter readings into the shared data model.
 * Does not issue any Modbus or SPI transactions; call after polling.
 *
 * SHT20 data is always written to inputRegisters regardless of meter type.
 * Modbus energy meter readings are only written for RTU meter types.
 * ATM90E32: poll_atm90e32() writes directly to readings[] — no copy needed.
 */
void update() {
    // SHT20 temperature/humidity into input registers — always present.
    inputRegisters[1] = sht20.getRawTemperature();
    inputRegisters[2] = sht20.getRawHumidity();

#if !defined(METER_TYPE_ATM90E32)
    // Energy meter readings — one entry per Modbus RTU meter.
    #if defined(METER_TYPE_DDS238)
    Modbus_DDS238* meters[MODBUS_NUM_METERS] = {&dds238_1, &dds238_2, &dds238_3};
    #elif defined(METER_TYPE_CHD130)
    Modbus_CHD130* meters[MODBUS_NUM_METERS] = {&chd130_1, &chd130_2, &chd130_3};
    #elif defined(METER_TYPE_DDSU666)
    Modbus_DDSU666* meters[MODBUS_NUM_METERS] = {&ddsu666_1, &ddsu666_2, &ddsu666_3};
    #endif

    for (int i = 0; i < MODBUS_NUM_METERS; i++) {
        readings[i].current       = meters[i]->getCurrent();
        readings[i].voltage       = meters[i]->getVoltage();
        readings[i].active_power  = meters[i]->getActivePower();
        readings[i].power_factor  = meters[i]->getPowerFactor();
        readings[i].frequency     = meters[i]->getFrequency();
        readings[i].total_energy  = meters[i]->getTotalEnergy();
        readings[i].export_energy = meters[i]->getExportEnergy();
        readings[i].import_energy = meters[i]->getImportEnergy();
        readings[i].timestamp_last_report = millis();
    }
#endif // !METER_TYPE_ATM90E32

#ifdef USB_PLOTTER
    // Record each meter's current in its own history buffer and emit a CSV
    // line to the Arduino Serial Plotter.
    // CSV columns: DATA, meter_index, timestamp_ms, current_A, voltage_V,
    //              active_power_kW, power_factor, frequency_Hz
    for (int i = 0; i < MODBUS_NUM_METERS; i++) {
        addCurrentReading(i, readings[i].current);
        Serial.printf("DATA,%d,%lu,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                      i, millis(),
                      readings[i].current, readings[i].voltage,
                      readings[i].active_power, readings[i].power_factor,
                      readings[i].frequency);
    }
#endif // USB_PLOTTER
}

// --------------------------------------------------------------------------
// SHT20 accessors — available to MQTT and other subsystems
// --------------------------------------------------------------------------

// Returns the last measured temperature in degrees Celsius.
float get_sht20_temperature() {
    return sht20.getTemperature();
}

// Returns the last measured relative humidity as a percentage (0–100).
float get_sht20_humidity() {
    return sht20.getHumidity();
}

// Returns the cumulative number of successful SHT20 poll responses.
// A non-zero value indicates that at least one valid reading has been received.
uint16_t get_sht20_success_count() {
    return sht20.getSuccessCount();
}

// --------------------------------------------------------------------------
// Polling helpers
// --------------------------------------------------------------------------

/**
 * Dump any bytes still in the UART RX buffer — called on SHT20 poll failure
 * to help diagnose partial or absent responses from the sensor.
 */
static void dump_uart_rx() {
    delay(200);
    uint8_t buf[16];
    uint8_t n = 0;
    while (_modbus1.available() && n < sizeof(buf)) buf[n++] = _modbus1.read();
#ifdef ENABLE_DEBUG
    if (n > 0) {
        Serial.printf("MODBUS RAW rx (%d bytes):", n);
        for (uint8_t i = 0; i < n; i++) Serial.printf(" 0x%02X", buf[i]);
        Serial.println();
    } else {
        Serial.println("MODBUS RAW rx: nothing — SHT20 sent no response");
    }
#else
    (void)n; // silence unused-variable warning in release builds
#endif
}

/**
 * Request fresh readings from all energy meters and push results to the
 * shared data model.
 */
void poll_energy_meters() {
#if defined(METER_TYPE_ATM90E32)
    poll_atm90e32();
    update();

#elif defined(METER_TYPE_DDS238)
    dds238_1.poll();
    dds238_2.poll();
    dds238_3.poll();
    update();

#elif defined(METER_TYPE_CHD130)
    chd130_1.poll();
    chd130_2.poll();
    chd130_3.poll();
    update();

#elif defined(METER_TYPE_DDSU666)
    ddsu666_1.poll();
    ddsu666_2.poll();
    ddsu666_3.poll();
    update();
#endif
}

/**
 * Request temperature and humidity from the SHT20 thermostat.
 * Always runs regardless of energy meter type — SHT20 uses RS-485 which is
 * independent from the ATM90E32 SPI bus.
 * On failure, dump_uart_rx() captures any stray bytes for diagnosis.
 * TODO: extend to an sht20_thermostats[] array for multiple sensors.
 */
void poll_thermostats() {
    // Only poll the SHT20; do NOT call update() here.
    // update() is called by poll_energy_meters() after meter data is fresh,
    // which prevents the full register-copy from running twice per cycle.
    uint8_t result = sht20.poll();
    if (result != 0x00) dump_uart_rx();
}

// --------------------------------------------------------------------------
// Main loop
// --------------------------------------------------------------------------

/**
 * Modbus master periodic service routine — call from main loop().
 * Fires a full poll cycle when ModbusMaster_pollrate milliseconds have elapsed
 * since the last cycle.
 */
void loop_modbus_master() {
    if (millis() - lastPollMillis > ModbusMaster_pollrate) {
        Serial.println("Starting poll cycle...");
        poll_thermostats();   // always poll SHT20 regardless of energy meter type
        poll_energy_meters();
        lastPollMillis = millis();
    }
}

#endif // ENABLE_MODBUS_MASTER
