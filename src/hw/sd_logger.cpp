#ifdef ENABLE_SD_SHT20_LOG

#include <SD.h>
#include <TimeLib.h>
#include <hw/sd_logger.h>
#include <hw/sd_card.h>
#include <metering/modbus_master.h>
#include <core/debug.h>

#define LOG_FILE      "/environmental_log.csv"
#define CSV_HEADER    "timestamp, sensor, temperature_c, humidity_%\n"

static unsigned long _lastLogMillis = 0;

void setup_sht20_csv_log() {
    if (!sd_card_available()) {
        DBUGLN("WARN - SHT20 CSV logger: SD not available");
        return;
    }
    if (!SD.exists(LOG_FILE)) {
        File f = SD.open(LOG_FILE, FILE_WRITE);
        if (f) {
            f.print(CSV_HEADER);
            f.close();
            DBUGLN("INFO - SHT20 CSV logger: created " LOG_FILE);
        } else {
            DBUGLN("WARN - SHT20 CSV logger: failed to create " LOG_FILE);
        }
    } else {
        DBUGLN("INFO - SHT20 CSV logger: appending to existing " LOG_FILE);
    }
}

void loop_sht20_csv_log() {
    if (!sd_card_available()) return;
    if (get_sht20_success_count() == 0) return;
    if (millis() - _lastLogMillis < SD_SHT20_LOG_INTERVAL_MS) return;

    _lastLogMillis = millis();

    File f = SD.open(LOG_FILE, FILE_APPEND);
    if (!f) {
        DBUGLN("WARN - SHT20 CSV logger: failed to open " LOG_FILE);
        return;
    }

    char row[48];
    snprintf(row, sizeof(row), "%lu, SHT20, %.1f, %.1f\n",
             (unsigned long)now(), get_sht20_temperature(), get_sht20_humidity());
    f.print(row);
    f.close();

    DBUGF("INFO - SHT20 log: T=%.1fC RH=%.1f%%",
          get_sht20_temperature(), get_sht20_humidity());
}

#endif // ENABLE_SD_SHT20_LOG
