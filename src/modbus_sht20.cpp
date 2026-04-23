#include <modbus_sht20.h>
#include <TimeLib.h>
#include <config.h>

#define PAUSE_ON_RAMP_LEVELS 30000

#if MODBUS_SERIAL_LOG
static const char* mbErrStr(uint8_t err) {
    switch (err) {
        case 0x01: return "ILLEGAL_FUNCTION";
        case 0x02: return "ILLEGAL_DATA_ADDRESS";
        case 0x03: return "ILLEGAL_DATA_VALUE";
        case 0x04: return "SLAVE_DEVICE_FAILURE";
        case 0xE0: return "INVALID_SLAVE_ID";
        case 0xE1: return "INVALID_FUNCTION";
        case 0xE2: return "TIMEOUT";
        case 0xE3: return "INVALID_CRC";
        default:   return "UNKNOWN";
    }
}
#endif

Modbus_SHT20::Modbus_SHT20() {
    temperature = -1;
    humidity = -1;
    fail_count = 0;
    success_count = 0;
}

uint8_t Modbus_SHT20::get_modbus_address() {
    return modbus_address;
}

void Modbus_SHT20::set_modbus_address(uint8_t addr) {
    modbus_address = addr;
}

uint16_t Modbus_SHT20::getFailCount()    { return fail_count; }
uint16_t Modbus_SHT20::getSuccessCount() { return success_count; }

float Modbus_SHT20::getTemperature() { return temperature/10.0; }
float Modbus_SHT20::getHumidity()    { return humidity/10.0; }
float Modbus_SHT20::getRawTemperature() { return temperature; }
float Modbus_SHT20::getRawHumidity()    { return humidity; }

void Modbus_SHT20::route_poll_response(uint16_t reg, uint16_t response) {
    switch (reg) {
        case rTEMPERATURE:
#if MODBUS_SERIAL_LOG
            Serial.printf("MODBUS SHT20: Temperature: %2.1fC\n", response/10.0);
#endif
            temperature = response;
            break;
        case rHUMIDITY:
#if MODBUS_SERIAL_LOG
            Serial.printf("MODBUS SHT20: Humidity %2.1f%%\n", response/10.0);
#endif
            humidity = response;
            break;
        default: break;
    }
}

uint8_t Modbus_SHT20::poll() {
    uint8_t result = readInputRegisters(rTEMPERATURE, 2);
    if (result == ku8MBSuccess) {
        success_count++;
        timestamp_last_report = now();
        temperature = getResponseBuffer(0);
        humidity = getResponseBuffer(1);
#if MODBUS_SERIAL_LOG
        Serial.printf("MODBUS SHT20 [addr:%d]: T=%2.1fC  RH=%2.1f%%  (ok:%d fail:%d)\n",
                      modbus_address, temperature/10.0, humidity/10.0, success_count, fail_count);
#endif
    } else {
        fail_count++;
        timestamp_last_failure = now();
#if MODBUS_SERIAL_LOG
        Serial.printf("MODBUS SHT20 [addr:%d]: POLL FAIL  err=0x%02X (%s)  (ok:%d fail:%d)\n",
                      modbus_address, result, mbErrStr(result), success_count, fail_count);
#endif
    }
    return result;
}

uint8_t Modbus_SHT20::query_register(uint16_t reg) {
    uint8_t result = readInputRegisters(reg, 1);
    if (result == ku8MBSuccess) {
        route_poll_response(reg, getResponseBuffer(0));
    } else {
        timestamp_last_failure = now();
#if MODBUS_SERIAL_LOG
        Serial.printf("MODBUS SHT20 [addr:%d]: query_register(%d) FAIL  err=0x%02X (%s)\n",
                      modbus_address, reg, result, mbErrStr(result));
#endif
    }
    return result;
}
