#include "circuitsetup_meter.h"

#include <ATM90E32.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <string.h>

#include "pins.h"

#include <TimeLib.h>
#include <data_model.h>

static constexpr int kNumChips = 2;
static constexpr int kNumChannels = 6;
static constexpr int kLongCap = 600;
static constexpr int kShortCap = 60;

static ATM90E32 g_meters[kNumChips];
static bool g_chipOk[kNumChips] = {false, false};

static float g_long[kLongCap][kNumChannels];
static float g_short[kShortCap][kNumChannels];
static int g_longHead = 0;
static int g_longCount = 0;
static int g_shortHead = 0;
static int g_shortCount = 0;

static unsigned long g_lastShortMs = 0;
static unsigned long g_lastLongMs = 0;
static unsigned long g_lastDiagMs = 0;

static uint16_t g_sys0[kNumChips] = {0, 0};
static uint16_t g_sys1[kNumChips] = {0, 0};
static uint16_t g_meter0[kNumChips] = {0, 0};
static uint16_t g_meter1[kNumChips] = {0, 0};
static float g_vA[kNumChips] = {0, 0};
static float g_vB[kNumChips] = {0, 0};
static float g_vC[kNumChips] = {0, 0};
static float g_freq[kNumChips] = {0, 0};

static void pushRow(float (*buf)[kNumChannels], int& head, int& count, int cap, const float* row6) {
    memcpy(buf[head], row6, sizeof(float) * kNumChannels);
    head = (head + 1) % cap;
    if (count < cap) {
        count++;
    }
}

void collectChannelSeries(const float (*buf)[kNumChannels], int head, int count, int cap, int channel,
                          JsonArray out) {
    if (count <= 0 || channel < 0 || channel >= kNumChannels) {
        return;
    }
    const int oldest = (count < cap) ? 0 : head;
    for (int i = 0; i < count; i++) {
        const int idx = (oldest + i) % cap;
        out.add(buf[idx][channel]);
    }
}

static int house_to_ct_channel(int houseMeterId) {
    int ch = houseMeterId % kNumChannels;
    if (ch < 0) {
        ch += kNumChannels;
    }
    return ch;
}

float circuitsetup_latest_amps(int ctChannel) {
    if (g_shortCount <= 0 || ctChannel < 0 || ctChannel >= kNumChannels) {
        return 0.0f;
    }
    const int r = (g_shortHead - 1 + kShortCap) % kShortCap;
    return g_short[r][ctChannel];
}

float circuitsetup_phase_voltage(int ctChannel) {
    if (ctChannel < 0 || ctChannel >= kNumChannels) {
        return 0.0f;
    }
    const int chip = ctChannel / 3;
    const int ph = ctChannel % 3;
    if (chip < 0 || chip >= kNumChips) {
        return 0.0f;
    }
    if (ph == 0) {
        return g_vA[chip];
    }
    if (ph == 1) {
        return g_vB[chip];
    }
    return g_vC[chip];
}

bool circuitsetup_chip_init_ok(int chipIndex) {
    if (chipIndex < 0 || chipIndex >= kNumChips) {
        return false;
    }
    return g_chipOk[chipIndex];
}

void circuitsetup_sync_powerdata_readings() {
    for (int i = 0; i < MODBUS_NUM_METERS; i++) {
        const float amps = circuitsetup_latest_amps(i);
        const float volts = circuitsetup_phase_voltage(i);
        readings[i].current = amps;
        readings[i].voltage = volts;
        readings[i].active_power = (amps * volts) / 1000.0f;
        readings[i].reactive_power = 0.0f;
        readings[i].power_factor = (volts > 1.0f && amps > 0.01f) ? 0.99f : 1.0f;
        readings[i].frequency = g_freq[i / 3];
        readings[i].timestamp_last_report = now();
    }
}

void setup_circuitsetup_meter() {
    pinMode(CIRCUITSETUP_IRQ0, INPUT);
    pinMode(CIRCUITSETUP_IRQ1, INPUT);

    // Inactive-high: other SPI devices on the shared bus (CAN, OLED) must not drive MISO during meter init.
#if defined(CAN0_CS)
    pinMode(CAN0_CS, OUTPUT);
    digitalWrite(CAN0_CS, HIGH);
#endif
#if defined(DISPLAY_CS_PIN)
    pinMode(DISPLAY_CS_PIN, OUTPUT);
    digitalWrite(DISPLAY_CS_PIN, HIGH);
#endif

    const int csPins[kNumChips] = {CIRCUITSETUP_METER_CS_A, CIRCUITSETUP_METER_CS_B};
    for (int i = 0; i < kNumChips; i++) {
        pinMode(csPins[i], OUTPUT);
        digitalWrite(csPins[i], HIGH);
    }

    for (int i = 0; i < kNumChips; i++) {
        ATM90E32::Config config;
        config.csPin = csPins[i];
        config.lineFrequency = ATM90E32::LINE_FREQUENCY_60HZ;
        config.currentPhases = ATM90E32::CURRENT_PHASES_3;
        config.pgaGain = ATM90E32::PGA_GAIN_1X;
        config.enableGainCalibration = false;
        config.enableOffsetCalibration = false;
        for (int p = 0; p < 3; p++) {
            config.phase[p].voltageGain = 7305;
            config.phase[p].currentGain = 27961;
            config.phase[p].referenceVoltage = 120.0f;
            config.phase[p].referenceCurrent = 50.0f;
        }
        g_chipOk[i] = g_meters[i].begin(config);
        if (!g_chipOk[i]) {
            Serial.printf("CircuitSetup: ATM90E32 chip %d init failed (CS GPIO %d)\n", i, csPins[i]);
        } else {
            Serial.printf("CircuitSetup: ATM90E32 chip %d OK (CS GPIO %d)\n", i, csPins[i]);
        }
    }
}

void loop_circuitsetup_meter() {
    const unsigned long now = millis();
    if (now - g_lastShortMs < 500) {
        return;
    }
    g_lastShortMs = now;

    float cur[kNumChannels];
    for (int chip = 0; chip < kNumChips; chip++) {
        if (!g_chipOk[chip]) {
            cur[chip * 3 + 0] = 0.0f;
            cur[chip * 3 + 1] = 0.0f;
            cur[chip * 3 + 2] = 0.0f;
            continue;
        }
        cur[chip * 3 + 0] = static_cast<float>(g_meters[chip].GetLineCurrentA());
        cur[chip * 3 + 1] = static_cast<float>(g_meters[chip].GetLineCurrentB());
        cur[chip * 3 + 2] = static_cast<float>(g_meters[chip].GetLineCurrentC());
    }
    pushRow(g_short, g_shortHead, g_shortCount, kShortCap, cur);

    if (now - g_lastLongMs >= 1000) {
        g_lastLongMs = now;
        pushRow(g_long, g_longHead, g_longCount, kLongCap, cur);
    }

    if (now - g_lastDiagMs >= 1000) {
        g_lastDiagMs = now;
        for (int chip = 0; chip < kNumChips; chip++) {
            if (!g_chipOk[chip]) {
                g_sys0[chip] = g_sys1[chip] = 0;
                g_meter0[chip] = g_meter1[chip] = 0;
                g_vA[chip] = g_vB[chip] = g_vC[chip] = 0;
                g_freq[chip] = 0;
                continue;
            }
            g_sys0[chip] = g_meters[chip].GetSysStatus0();
            g_sys1[chip] = g_meters[chip].GetSysStatus1();
            g_meter0[chip] = g_meters[chip].GetMeterStatus0();
            g_meter1[chip] = g_meters[chip].GetMeterStatus1();
            g_vA[chip] = static_cast<float>(g_meters[chip].GetLineVoltageA());
            g_vB[chip] = static_cast<float>(g_meters[chip].GetLineVoltageB());
            g_vC[chip] = static_cast<float>(g_meters[chip].GetLineVoltageC());
            g_freq[chip] = static_cast<float>(g_meters[chip].GetFrequency());
        }
    }
}

String circuitsetup_api_json(int houseMeterId) {
    const int channel = house_to_ct_channel(houseMeterId);
    const int chipIndex = channel / 3;
    const int phaseIndex = channel % 3;
    static const char* kPhaseLetters = "ABC";

    DynamicJsonDocument doc(32768);
    doc["houseMeterId"] = houseMeterId;
    doc["ctChannel"] = channel;
    doc["ctLabel"] = String("CT") + String(channel + 1);
    doc["chipIndex"] = chipIndex;
    doc["phaseLetter"] = String(kPhaseLetters[phaseIndex]);
    doc["intervalLongMs"] = 1000;
    doc["intervalShortMs"] = 500;
    doc["longCapacity"] = kLongCap;
    doc["shortCapacity"] = kShortCap;
    doc["longSamples"] = g_longCount;
    doc["shortSamples"] = g_shortCount;
    doc["uptimeMs"] = millis();

    JsonArray longArr = doc["longAmps"].to<JsonArray>();
    collectChannelSeries(g_long, g_longHead, g_longCount, kLongCap, channel, longArr);
    JsonArray shortArr = doc["shortAmps"].to<JsonArray>();
    collectChannelSeries(g_short, g_shortHead, g_shortCount, kShortCap, channel, shortArr);

    doc["latestAmps"] = (g_shortCount > 0)
                            ? g_short[(g_shortHead - 1 + kShortCap) % kShortCap][channel]
                            : 0.0f;

    JsonArray allLatest = doc["latestAllAmps"].to<JsonArray>();
    if (g_shortCount > 0) {
        const int r = (g_shortHead - 1 + kShortCap) % kShortCap;
        for (int c = 0; c < kNumChannels; c++) {
            allLatest.add(g_short[r][c]);
        }
    } else {
        for (int c = 0; c < kNumChannels; c++) {
            allLatest.add(0.0f);
        }
    }

    JsonObject diag = doc["diagnostics"].to<JsonObject>();
    diag["spiMosi"] = CIRCUITSETUP_SPI_MOSI;
    diag["spiMiso"] = CIRCUITSETUP_SPI_MISO;
    diag["spiSck"] = CIRCUITSETUP_SPI_SCK;
    diag["csGpioA"] = CIRCUITSETUP_METER_CS_A;
    diag["csGpioB"] = CIRCUITSETUP_METER_CS_B;
    diag["irq0Level"] = digitalRead(CIRCUITSETUP_IRQ0);
    diag["irq1Level"] = digitalRead(CIRCUITSETUP_IRQ1);
    diag["hint"] =
        "ATM90 uses global SPI on SCK/MISO/MOSI from pins.h (same bus as CAN/OLED). If initOk stays "
        "false: verify CS-A/CS-B GPIOs vs 865B netlist, 3V3 to meter, and line voltage inputs. "
        "Idle CS lines (CAN, OLED, both meter CS) must be high.";

    JsonArray chips = diag["chips"].to<JsonArray>();
    const int csPins[kNumChips] = {CIRCUITSETUP_METER_CS_A, CIRCUITSETUP_METER_CS_B};
    for (int i = 0; i < kNumChips; i++) {
        JsonObject c = chips.add<JsonObject>();
        c["index"] = i;
        c["csGpio"] = csPins[i];
        c["initOk"] = g_chipOk[i];
        c["sysStatus0"] = g_sys0[i];
        c["sysStatus1"] = g_sys1[i];
        c["meterStatus0"] = g_meter0[i];
        c["meterStatus1"] = g_meter1[i];
        c["voltageA"] = g_vA[i];
        c["voltageB"] = g_vB[i];
        c["voltageC"] = g_vC[i];
        c["frequencyHz"] = g_freq[i];
    }

    String out;
    serializeJson(doc, out);
    return out;
}
