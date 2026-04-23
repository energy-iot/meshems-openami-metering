/**
 * @file modbus_master.cpp
 * @brief Modbus master implementation for SHT20 temperature/humidity sensors
 */

#include <HardwareSerial.h>
#include <modbus.h>
#include <pins.h>
#include <data_model.h>
#include <config.h>
#include <math.h>  // For sin function in test data

// Poll every 10 seconds (300000ms = 5 mins for production)
// Changed to 500ms for more stable operation
// Make the polling interval adjustable and accessible from other files
//unsigned int POLL_INTERVAL = 100;  // moved to config.h 

// ==================== Modbus Device Addresses ====================
// during staging of subpanel must stage each modbus meter with its assigned node number - in future could use qr code sticker per meter for faster staging of a subpanel
#define THERMOSTAT_1_ADDR 0x01    // when no meters shared on same rs485 modbus RTU line
//#define THERMOSTAT_1_ADDR 0x99  // at staging of subpanel set temp/humid sensor as modbus node num 99 so never conflicts

// DO NOT USE FOR REAL , either 1 tenant meter per phase or all 3 meters on same phase 
#define DDS238_1_ADDR 0x50
#define DDS238_2_ADDR 0x51
#define DDS238_3_ADDR 0x52


// 3 meter subpanel , either 1 tenant meter per phase or all 3 meters on same phase 
// #define DDS238_1_ADDR 0x01
// #define DDS238_2_ADDR 0x02
// #define DDS238_3_ADDR 0x03
// uncomment for 6 meter subpanel , either n tenant meters per phase, or all  meters on same phase 
//#define DDS238_1_ADDR 0x04
//#define DDS238_2_ADDR 0x05
//#define DDS238_3_ADDR 0x06
// uncomment for 9 meter subpanel , either n tenant meters per phase, or all  meters on same phase 
//#define DDS238_1_ADDR 0x07
//#define DDS238_2_ADDR 0x08
//#define DDS238_3_ADDR 0x09
// NOTE 3/6/9 multitenant subpanels can be cookie cutter options - 3 phase so x tenants/meters per phase typical, assume subanels in multiples of 3


// ==================== Serial Interface Setup ====================
HardwareSerial _modbus1(1); // UART1 routed to RS485_RX_1/TX_1 via ESP32-S3 GPIO matrix
//SoftwareSerial *modbus2(RS485_RX_2, RS485_TX_2); // uncomment when 2 modbus masters are to be used on the EMS PCB
// TODO merge Canbus support and Modbus Client in other EMS branch to here ModCan

// Temperature/humidity sensor
Modbus_SHT20 sht20;

// Energy Meter (HIKING DDS238)
Modbus_DDS238 dds238_1;
//UNcomment for the 3-meter subpanels
Modbus_DDS238 dds238_2;
Modbus_DDS238 dds238_3;
//UNcomment for the 6-meter subpanels
//Modbus_DDS238 dds238_4;
//Modbus_DDS238 dds238_5;
//Modbus_DDS238 dds238_6;
//UNcomment for the 9-meter subpanels
//Modbus_DDS238 dds238_7;
//Modbus_DDS238 dds238_8;
//Modbus_DDS238 dds238_9;

//Modbus_DDS238* dds238_meters[MODBUS_NUM_METERS] = {&dds238_1}; // Array of single 3 phase or single 1 phase tenant Modbus meters
Modbus_DDS238* dds238_meters[MODBUS_NUM_METERS] = {&dds238_1, &dds238_2, &dds238_3}; // Array of single 3 phase or single 1 phase tenant Modbus meters

// Uncomment for the 3/6/9 single phase meter 
ModbusMaster* meters[MODBUS_NUM_METERS] = {&dds238_1, &dds238_2, &dds238_3}; // add to the array for the 3 multi-meter boxes
//ModbusMaster* meters[MODBUS_NUM_METERS] = {&dds238_1, &dds238_2, &dds238_3, &dds238_4, &dds238_5, &dds238_6}; // add to the array for the 3 multi-meter boxes
//ModbusMaster* meters[MODBUS_NUM_METERS] = {&dds238_1, &dds238_2, &dds238_3, &dds238_4, &dds238_5, &dds238_6, &dds238_7, &dds238_8, &dds238_9}; // add to the array for the 3 multi-meter boxes
// Timing variables
unsigned long lastPollMillis, lastEVSEMillis, lastEVSEChargingMillis = 0;

/**
 * Initialize SHT20 temperature/humidity sensor
 */
void setup_sht20() {
    Serial.printf("SETUP: MODBUS: SHT20 #1: address:%d\n", THERMOSTAT_1_ADDR);
    sht20.set_modbus_address(THERMOSTAT_1_ADDR);
    sht20.begin(THERMOSTAT_1_ADDR, _modbus1);    // node number 99 so doesnt conflict with meters at 1-n
}

void setup_dds238() {
   Serial.printf("SETUP: MODBUS: DDS238 #1: address:%d\n", DDS238_1_ADDR);
   dds238_1.set_modbus_address(DDS238_1_ADDR);
   dds238_2.set_modbus_address(DDS238_2_ADDR);
   dds238_3.set_modbus_address(DDS238_3_ADDR);
   //dds238_4.set_modbus_address(DDS238_4_ADDR);
   //dds238_5.set_modbus_address(DDS238_5_ADDR);
   //dds238_6.set_modbus_address(DDS238_6_ADDR);
   dds238_1.begin(DDS238_1_ADDR, _modbus1);
   dds238_2.begin(DDS238_2_ADDR, _modbus1);
   dds238_3.begin(DDS238_3_ADDR, _modbus1);
    //dds238_4.begin(DDS238_4_ADDR, _modbus1);
    //dds238_5.begin(DDS238_5_ADDR, _modbus1);
    //dds238_6.begin(DDS238_6_ADDR, _modbus1);
    //dds238_7.begin(DDS238_7_ADDR, _modbus1);
    //dds238_8.begin(DDS238_8_ADDR, _modbus1);
    //dds238_9.begin(DDS238_9_ADDR, _modbus1);
}

/**
 * Initialize all Modbus clients
 */
void setup_modbus_clients() {
    //setup_thermostats();  // Future expansion for modbus building thermostat
    //setup_dtm();          // Future expansion Building mains
    //TODO support thermostat and mete on same modbus master daisy chain
#if MODBUS_ENABLE_SHT20
    setup_sht20();          // Initialize SHT20 temp/humid modbus sensor
#endif
    //setup_evse();         // Future expansion multitenant EV charging
    setup_dds238();         // Initialize single phase meters , note they can be wired on separate phases
}

/**
 * Initialize Modbus master interface
 */


void setup_modbus_master() {
    // Reset GPIO pins for 2 ports of RS485
    gpio_reset_pin(RS485_RX_1);
    gpio_reset_pin(RS485_TX_1);
    gpio_reset_pin(RS485_RX_2);
    gpio_reset_pin(RS485_TX_2);

    // UART1 routed through ESP32-S3 GPIO matrix — no rewiring needed, interrupt-safe with WiFi
    _modbus1.begin(9600, SERIAL_8N1, RS485_RX_1, RS485_TX_1);

    int rxLevel = digitalRead(RS485_RX_1);
    Serial.printf("MODBUS SETUP: RS485_RX_1 (GPIO%d) idle level = %s (expect HIGH)\n",
                  RS485_RX_1, rxLevel ? "HIGH" : "LOW");

    _modbus1.begin(9600, SERIAL_8N1, RS485_RX_1, RS485_TX_1);

    // Setup connected devices
    setup_modbus_clients();
}

/**
 * Update data model with current sensor readings
 */
void update() {
#if MODBUS_ENABLE_SHT20
    inputRegisters[0] = sht20.getTemperature();
    inputRegisters[1] = sht20.getHumidity();
#else
    inputRegisters[0] = 0.0f;
    inputRegisters[1] = 0.0f;
#endif
    
    // Get all measurements from meters
    for(int i=0;i<MODBUS_NUM_METERS;i++) {
        readings[i].current = dds238_meters[i]->getCurrent(); // Current
        readings[i].voltage = dds238_meters[i]->getVoltage(); // Voltage
        readings[i].active_power = dds238_meters[i]->getActivePower(); // Active Power
        readings[i].power_factor = dds238_meters[i]->getPowerFactor(); // Power Factor
        readings[i].frequency = dds238_meters[i]->getFrequency(); // Frequency
        readings[i].total_energy = dds238_meters[i]->getTotalEnergy(); // Total Energy
        readings[i].export_energy = dds238_meters[i]->getExportEnergy(); // Export Energy
        readings[i].import_energy = dds238_meters[i]->getImportEnergy(); // Import Energy
        
        // If readings are zero or invalid, generate test data
        /*
        if (readings[i].current <= 0.001) {
            // Simulate patterns for testing
            unsigned long t = millis();
            readings[i].current = 2.5 + 2.0 * sin(t / 2000.0);
            readings[i].voltage = 230.0 + 5.0 * sin(t / 1500.0);
            readings[i].active_power = readings[i].current * readings[i].voltage * 0.95;
            readings[i].power_factor = 0.95 + 0.05 * sin(t / 3000.0);
            readings[i].frequency = 50.0 + 0.1 * sin(t / 4000.0);
            Serial.printf("MODBUS METER using simulated values\n");
        } else {
            Serial.printf("MODBUS METER using real values\n");
        }*/
    
    }
    
    // Add the reading to our history buffer for timeline plotting
    //TODO plot all meters, not just the first one
    addCurrentReading(readings[0].current); 
    
    // Send CSV formatted data over USB for plotting on computer
    // Format: DATA,timestamp,current,voltage,power,pf,freq
    //TODO plot all meters, not just the first one
    Serial.printf("DATA,%lu,%.3f,%.3f,%.3f,%.3f,%.3f\n", 
                 millis(), readings[0].current, readings[0].voltage, readings[0].active_power, readings[0].power_factor, readings[0].frequency);
}

void poll_energy_meters() {
    // Poll each DDS238 meter
    for(int i = 0; i < MODBUS_NUM_METERS; i++) {
        dds238_meters[i]->poll();
    }
    // Update data model cache with latest readings
    update();
}

void poll_thermostats() {
#if MODBUS_ENABLE_SHT20
    uint8_t result = sht20.poll();
    if (result != 0x00) {
        // Wait briefly for any late-arriving bytes, then print whatever the UART has
        delay(200);
        uint8_t n = 0;
        uint8_t buf[16];
        while (_modbus1.available() && n < sizeof(buf)) buf[n++] = _modbus1.read();
        if (n > 0) {
            Serial.printf("MODBUS RAW rx (%d bytes):", n);
            for (uint8_t i = 0; i < n; i++) Serial.printf(" 0x%02X", buf[i]);
            Serial.println();
        } else {
            Serial.println("MODBUS RAW rx: nothing — SHT20 sent no response");
        }
    }
#endif
    // TODO: extend to sht20_thermostats[] array for multiple sensors
    update();
}

/**
 * Main polling loop for Modbus communication
 */
void loop_modbus_master() {
    if (millis() - lastPollMillis > ModbusMaster_pollrate) {
        Serial.println("Starting poll cycle...");
        poll_thermostats(); // Poll thermostat/environmental  sensors such as cabinet temp/humid/pressure/ , wire mains and evse temps etc
        // poll_energy_meters(); // Poll energy meters
        // TODO poll other modbus device on same link
        lastPollMillis = millis();
    }
}