#pragma once
#ifdef ENABLE_SD_SHT20_LOG

#ifndef ENABLE_SD_CARD
  #error "ENABLE_SD_SHT20_LOG requires ENABLE_SD_CARD."
#endif
#ifndef ENABLE_MODBUS_MASTER
  #error "ENABLE_SD_SHT20_LOG requires ENABLE_MODBUS_MASTER."
#endif

#ifndef SD_SHT20_LOG_INTERVAL_MS
  #define SD_SHT20_LOG_INTERVAL_MS 60000UL
#endif

void setup_sht20_csv_log();
void loop_sht20_csv_log();

#endif // ENABLE_SD_SHT20_LOG
