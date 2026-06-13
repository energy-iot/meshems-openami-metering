#ifdef ENABLE_SD_CARD

#include <SD.h>
#include <SPI.h>
#include <hw/sd_card.h>
#include <core/pins.h>
#include <core/debug.h>

static bool _sd_available = false;

#ifdef ENABLE_SD_CARD_DEBUG
void sd_list_dir(const char* path, uint8_t depth) {
    File dir = SD.open(path);
    if (!dir) return;

    char indent[17] = {0};
    for (uint8_t i = 0; i < depth && i < 8; i++) {
        indent[i * 2]     = ' ';
        indent[i * 2 + 1] = ' ';
    }

    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        if (entry.isDirectory()) {
            DBUGF("SD    %s[%s]", indent, entry.name());
            sd_list_dir(entry.path(), depth + 1);
        } else {
            DBUGF("SD    %s%-24s  %lu bytes", indent, entry.name(), (unsigned long)entry.size());
        }
        entry.close();
    }
    dir.close();
}
#endif

bool setup_sd_card() {
    // Re-initialize SPI to include MISO. SD_CLK and SD_MOSI are shared with
    // the OLED bus on V3 (GPIO 12 / GPIO 11), so this is safe — write-only
    // peripherals on the same bus are unaffected by adding the MISO line.
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    if (!SD.begin(SD_CS)) {
        DBUGLN("WARN - SD card init failed or no card inserted");
        _sd_available = false;
        return false;
    }

    if (SD.cardType() == CARD_NONE) {
        DBUGLN("WARN - SD card: no card detected");
        _sd_available = false;
        return false;
    }

    _sd_available = true;
    DBUGF("INFO - SD card ready (%lluMB)", SD.cardSize() / (1024ULL * 1024ULL));

#ifdef ENABLE_SD_CARD_DEBUG
    const char* typeStr = "UNKNOWN";
    switch (SD.cardType()) {
        case CARD_MMC:  typeStr = "MMC";  break;
        case CARD_SD:   typeStr = "SD";   break;
        case CARD_SDHC: typeStr = "SDHC/SDXC"; break;
        default: break;
    }
    DBUGF("SD  type       : %s", typeStr);
    DBUGF("SD  card size  : %llu MB", SD.cardSize() / (1024ULL * 1024ULL));
    DBUGF("SD  total bytes: %llu", SD.totalBytes());
    DBUGF("SD  used bytes : %llu", SD.usedBytes());
    DBUGF("SD  free bytes : %llu", SD.totalBytes() - SD.usedBytes());
    DBUGLN("SD  contents   :");
    sd_list_dir("/", 0);
#endif

    return true;
}

bool sd_card_available() {
    return _sd_available;
}

#endif // ENABLE_SD_CARD
