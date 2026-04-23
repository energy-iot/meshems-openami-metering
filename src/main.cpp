/**
 * @file main.cpp
 * @brief Main application entry point for the Energy IoT Source firmware
 * @author Doug Mendonca, Liam O'Brien
 * @date April 18, 2025
 * 
 * This file contains the setup and main loop for the Energy IoT EMS Dev Platform 
 * controlling the OLED display, MODBUS interfaces, CAN bus interface, 
 * and button interfaces.
 * 
 *  _____                             _____ _____ _____   _____                  _____                          
 * |  ___|                           |_   _|  _  |_   _| |  _  |                /  ___|                         
 * | |__ _ __   ___ _ __ __ _ _   _    | | | | | | | |   | | | |_ __   ___ _ __ \ `--.  ___  _   _ _ __ ___ ___ 
 * |  __| '_ \ / _ \ '__/ _` | | | |   | | | | | | | |   | | | | '_ \ / _ \ '_ \ `--. \/ _ \| | | | '__/ __/ _ \
 * | |__| | | |  __/ | | (_| | |_| |  _| |_\ \_/ / | |   \ \_/ / |_) |  __/ | | /\__/ / (_) | |_| | | | (_|  __/
 * \____/_| |_|\___|_|  \__, |\__, |  \___/ \___/  \_/    \___/| .__/ \___|_| |_\____/ \___/ \__,_|_|  \___\___|
 *                       __/ | __/ |                           | |                                              
 *                      |___/ |___/                            |_|                                              
 *
 * Copyright 2025
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Arduino.h>
#include <math.h>
#include <SPI.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <config.h>
#if !HACK_LAB_SKIP_MODBUS
#include <modbus.h>     // Modbus communication protocols
#endif
#include <buttons.h>    // Button input handling
#include <display.h>    // SH1106 OLED display
#include <console.h>    // Console UI for the display
#include <can.h>        // Implementation of CAN bus communication
#include <i2c_ssr_bank.h>
#include <wifi.h>
#include <mqtt_client.h>
#include <data_model.h>
#include <circuitsetup_meter.h>
#include <pins.h>

static AsyncWebServer server(80);
static const uint8_t kBoards = 1;
static const uint8_t kChannelsPerBoard = 10;
static const uint8_t kHardwareRelayChannels = 8;
static const uint8_t kHistoryHours = 24;
static bool relayShadow[kBoards][kChannelsPerBoard] = {};
static const char* kTenantLabels[kChannelsPerBoard] = {
    "Kato Family", "Nankya Family", "Ssewankambo Family", "Nabirye Family", "Okello Family",
    "Achieng Family", "Mugisha Family", "Namusoke Family", "Byaruhanga Family", "Atim Family"
};
static const double kHouseLatLon[kChannelsPerBoard][2] = {
    {0.3626654865345586, 32.53551516882119},
    {0.3626195191078696, 32.53559312333386},
    {0.36257188497217857, 32.53567085303205},
    {0.36251902970261446, 32.53575027281464},
    {0.3625701573901022, 32.53580467541863},
    {0.3625489462228964, 32.53590189711509},
    {0.3624890839504084, 32.535863006262296},
    {0.36242760831352216, 32.53582958827747},
    {0.3623963489621337, 32.535910395715796},
    {0.36228129520905056, 32.535983770700994}
};

static float simulated_kwh(uint8_t board, uint8_t meter, uint8_t hour) {
    const float m = float(meter);
    const float phase = m * 0.55f;  // stagger daily curve per tenant (dashboard chart layering)
    float base = 0.28f + (0.055f * m) + (0.10f * float(board));
    float daywave = 0.11f * sinf((float(hour) / 24.0f) * 6.2831853f + phase);
    float evening = (hour >= 18 && hour <= 22) ? (0.14f + 0.025f * fmodf(m, 4.0f)) : 0.0f;
    float midday = (hour >= 11 && hour <= 15) ? (0.04f * sinf(m * 0.8f + float(hour) * 0.15f)) : 0.0f;
    return max(0.05f, base + daywave + evening + midday);
}

static float daily_kwh_total(uint8_t board, uint8_t meter) {
    float total = 0.0f;
    for (uint8_t hour = 0; hour < kHistoryHours; hour++) {
        total += simulated_kwh(board, meter, hour);
    }
    return total;
}

static constexpr double kDegToRad = 0.017453292519943295;

/** Narrow horizontal ribbon in the sky (fill-extrusion base..height) along a ground segment. */
static void append_ribbon_segment(JsonArray features, double lat0, double lon0, double lat1, double lon1,
                                  double halfWidthM, float baseM, float topM, const char* assetType,
                                  const char* ribbonName) {
    const double midLat = (lat0 + lat1) * 0.5;
    const double cosLat = cos(midLat * kDegToRad);
    const double clampedCos = (cosLat > 0.2) ? cosLat : 0.2;

    const double e1 = (lon1 - lon0) * 111320.0 * clampedCos;
    const double n1 = (lat1 - lat0) * 111320.0;
    const double len = sqrt(e1 * e1 + n1 * n1);
    if (len < 0.05) {
        return;
    }
    const double pe = (-n1 / len) * halfWidthM;
    const double pn = (e1 / len) * halfWidthM;

    const double eA = pe;
    const double nA = pn;
    const double eB = -pe;
    const double nB = -pn;
    const double eC = e1 - pe;
    const double nC = n1 - pn;
    const double eD = e1 + pe;
    const double nD = n1 + pn;

    JsonObject f = features.add<JsonObject>();
    f["type"] = "Feature";
    JsonObject props = f["properties"].to<JsonObject>();
    props["assetType"] = assetType;
    if (ribbonName != nullptr) {
        props["name"] = ribbonName;
    }
    props["extrudeBase"] = baseM;
    props["extrudeHeight"] = topM;

    JsonObject geom = f["geometry"].to<JsonObject>();
    geom["type"] = "Polygon";
    JsonArray rings = geom["coordinates"].to<JsonArray>();
    JsonArray ring = rings.add<JsonArray>();

    auto enu_corner = [&](double e, double n) {
        const double la = lat0 + n / 111320.0;
        const double lo = lon0 + e / (111320.0 * clampedCos);
        JsonArray pt = ring.add<JsonArray>();
        pt.add(lo);
        pt.add(la);
    };
    enu_corner(eA, nA);
    enu_corner(eB, nB);
    enu_corner(eC, nC);
    enu_corner(eD, nD);
    enu_corner(eA, nA);
}

static void append_house_extrusion(JsonArray features, double lat, double lon, uint8_t board, uint8_t meter,
                                   bool relayState, float dailyKwh) {
    constexpr double kHalfM = 5.0;
    const double cosLat = cos(lat * kDegToRad);
    const double clampedCos = (cosLat > 0.2) ? cosLat : 0.2;
    const double dLat = kHalfM / 111320.0;
    const double dLon = kHalfM / (111320.0 * clampedCos);

    JsonObject f = features.add<JsonObject>();
    f["type"] = "Feature";
    f["id"] = String("hs-") + String((int)board) + "-" + String((int)meter);
    JsonObject p = f["properties"].to<JsonObject>();
    p["assetType"] = "houseExtrusion";
    p["boardId"] = board;
    p["meterId"] = meter;
    p["relayChannel"] = meter;
    p["name"] = String("House-") + (meter + 1);
    p["tenantLabel"] = kTenantLabels[meter];
    p["dailyKwh"] = dailyKwh;
    p["relayState"] = relayState;
    p["extrudeBase"] = 0.0f;
    p["extrudeHeight"] = 3.6f;

    JsonObject geom = f["geometry"].to<JsonObject>();
    geom["type"] = "Polygon";
    JsonArray rings = geom["coordinates"].to<JsonArray>();
    JsonArray ring = rings.add<JsonArray>();
    auto corner = [&](double la, double lo) {
        JsonArray pt = ring.add<JsonArray>();
        pt.add(lo);
        pt.add(la);
    };
    corner(lat - dLat, lon - dLon);
    corner(lat - dLat, lon + dLon);
    corner(lat + dLat, lon + dLon);
    corner(lat + dLat, lon - dLon);
    corner(lat - dLat, lon - dLon);
}

static void append_pole_extrusion(JsonArray features, double lat, double lon, uint8_t board, const String& name,
                                  float heightM) {
    constexpr double kHalfM = 0.45;
    const double cosLat = cos(lat * kDegToRad);
    const double clampedCos = (cosLat > 0.2) ? cosLat : 0.2;
    const double dLat = kHalfM / 111320.0;
    const double dLon = kHalfM / (111320.0 * clampedCos);

    JsonObject f = features.add<JsonObject>();
    f["type"] = "Feature";
    f["id"] = String("ps-") + String((int)board);
    JsonObject p = f["properties"].to<JsonObject>();
    p["assetType"] = "poleExtrusion";
    p["boardId"] = board;
    p["name"] = name;
    p["height_m"] = heightM;
    p["extrudeBase"] = 0.0f;
    p["extrudeHeight"] = heightM;

    JsonObject geom = f["geometry"].to<JsonObject>();
    geom["type"] = "Polygon";
    JsonArray rings = geom["coordinates"].to<JsonArray>();
    JsonArray ring = rings.add<JsonArray>();
    auto corner = [&](double la, double lo) {
        JsonArray pt = ring.add<JsonArray>();
        pt.add(lo);
        pt.add(la);
    };
    corner(lat - dLat, lon - dLon);
    corner(lat - dLat, lon + dLon);
    corner(lat + dLat, lon + dLon);
    corner(lat + dLat, lon - dLon);
    corner(lat - dLat, lon - dLon);
}

static void append_geojson(JsonArray features) {
    {
        JsonObject lineFeature = features.add<JsonObject>();
        lineFeature["type"] = "Feature";
        JsonObject props = lineFeature["properties"].to<JsonObject>();
        props["assetType"] = "line";
        props["name"] = "Court feeder";

        JsonObject geom = lineFeature["geometry"].to<JsonObject>();
        geom["type"] = "LineString";
        JsonArray coords = geom["coordinates"].to<JsonArray>();

        JsonArray c0 = coords.add<JsonArray>();
        c0.add(32.53545); c0.add(0.36270); c0.add(6.0);
        JsonArray c1 = coords.add<JsonArray>();
        c1.add(32.53605); c1.add(0.36225); c1.add(6.0);
    }
    append_ribbon_segment(features, 0.36270, 32.53545, 0.36225, 32.53605, 0.45, 5.6f, 6.05f, "lineAir",
                          "Feeder aerial");

    for (uint8_t board = 0; board < kBoards; board++) {
        double centerLon = 0.0;
        double centerLat = 0.0;
        for (uint8_t house = 0; house < kChannelsPerBoard; house++) {
            centerLat += kHouseLatLon[house][0];
            centerLon += kHouseLatLon[house][1];
        }
        centerLat /= (double)kChannelsPerBoard;
        centerLon /= (double)kChannelsPerBoard;

        JsonObject courtFeature = features.add<JsonObject>();
        courtFeature["type"] = "Feature";
        JsonObject courtProps = courtFeature["properties"].to<JsonObject>();
        courtProps["assetType"] = "court";
        courtProps["boardId"] = board;
        courtProps["name"] = "Ssezibwa Homes Court";
        JsonObject courtGeom = courtFeature["geometry"].to<JsonObject>();
        courtGeom["type"] = "LineString";
        JsonArray courtCoords = courtGeom["coordinates"].to<JsonArray>();
        for (uint8_t house = 0; house < kChannelsPerBoard; house++) {
            JsonArray p = courtCoords.add<JsonArray>();
            p.add(kHouseLatLon[house][1]); // lon
            p.add(kHouseLatLon[house][0]); // lat
            p.add(0.5f);
        }
        JsonArray p0 = courtCoords.add<JsonArray>();
        p0.add(kHouseLatLon[0][1]);
        p0.add(kHouseLatLon[0][0]);
        p0.add(0.5f);

        for (uint8_t i = 0; i < kChannelsPerBoard; i++) {
            const uint8_t j = (i + 1) % kChannelsPerBoard;
            append_ribbon_segment(features, kHouseLatLon[i][0], kHouseLatLon[i][1], kHouseLatLon[j][0],
                                  kHouseLatLon[j][1], 0.35, 4.0f, 4.4f, "courtAir", "Court LV loop");
        }

        JsonObject poleFeature = features.add<JsonObject>();
        poleFeature["type"] = "Feature";
        poleFeature["id"] = String("pn-") + String((int)board);
        JsonObject props = poleFeature["properties"].to<JsonObject>();
        props["assetType"] = "streetPoleEMS";
        props["boardId"] = board;
        props["name"] = String("StreetPoleEMS-") + board;
        props["height_m"] = 7.5;

        JsonObject geom = poleFeature["geometry"].to<JsonObject>();
        geom["type"] = "Point";
        JsonArray coord = geom["coordinates"].to<JsonArray>();
        coord.add(centerLon);
        coord.add(centerLat);
        coord.add(7.5);

        append_pole_extrusion(features, centerLat, centerLon, board, String("StreetPoleEMS-") + board, 7.5f);

        for (uint8_t meter = 0; meter < kChannelsPerBoard; meter++) {
            JsonObject meterFeature = features.add<JsonObject>();
            meterFeature["type"] = "Feature";
            meterFeature["id"] = String("hn-") + String((int)board) + "-" + String((int)meter);
            JsonObject meterProps = meterFeature["properties"].to<JsonObject>();
            meterProps["assetType"] = "house";
            meterProps["boardId"] = board;
            meterProps["meterId"] = meter;
            meterProps["relayChannel"] = meter;
            meterProps["name"] = String("House-") + (meter + 1);
            meterProps["tenantLabel"] = kTenantLabels[meter];
            meterProps["dailyKwh"] = daily_kwh_total(board, meter);
            meterProps["relayState"] = relayShadow[board][meter];

            JsonObject meterGeom = meterFeature["geometry"].to<JsonObject>();
            meterGeom["type"] = "Point";
            JsonArray meterCoord = meterGeom["coordinates"].to<JsonArray>();
            meterCoord.add(kHouseLatLon[meter][1]); // lon
            meterCoord.add(kHouseLatLon[meter][0]); // lat
            meterCoord.add(1.8);

            append_house_extrusion(features, kHouseLatLon[meter][0], kHouseLatLon[meter][1], board, meter,
                                   relayShadow[board][meter], daily_kwh_total(board, meter));
        }
    }
}

static String build_dashboard_json() {
    DynamicJsonDocument doc(49152);

    doc["siteId"] = getDeviceID();
    doc["generatedAtMs"] = millis();
    doc["meterSource"] = "circuitsetup";
    JsonArray chipsOk = doc["circuitSetupChipsOk"].to<JsonArray>();
    chipsOk.add(circuitsetup_chip_init_ok(0));
    chipsOk.add(circuitsetup_chip_init_ok(1));
    doc["activeBoardForHardware"] = 0;
    JsonObject villageCenter = doc["villageCenter"].to<JsonObject>();
    villageCenter["lon"] = 32.53578;
    villageCenter["lat"] = 0.36251;
    villageCenter["zoom"] = 18;

    JsonObject geojson = doc["utilityGeoJson"].to<JsonObject>();
    geojson["type"] = "FeatureCollection";
    JsonArray features = geojson["features"].to<JsonArray>();
    append_geojson(features);

    JsonArray boards = doc["boards"].to<JsonArray>();
    for (uint8_t board = 0; board < kBoards; board++) {
        JsonObject boardObj = boards.add<JsonObject>();
        boardObj["boardId"] = board;
        boardObj["name"] = String("StreetPoleEMS-") + board;
        boardObj["hardwareMapped"] = board == 0;

        JsonArray meters = boardObj["meters"].to<JsonArray>();
        for (uint8_t channel = 0; channel < kChannelsPerBoard; channel++) {
            JsonObject meter = meters.add<JsonObject>();
            meter["meterId"] = channel;
            meter["relayChannel"] = channel;
            meter["tenantLabel"] = kTenantLabels[channel];
            meter["relayState"] = relayShadow[board][channel];
            meter["dailyKwhTotal"] = daily_kwh_total(board, channel);

            const int ct = static_cast<int>(channel) % 6;
            meter["ctChannel"] = ct;
            meter["ctLabel"] = String("CT") + (ct + 1);
            {
                const float la = circuitsetup_latest_amps(ct);
                const float lv = circuitsetup_phase_voltage(ct);
                meter["liveAmps"] = la;
                meter["liveVolts"] = lv;
                meter["liveKwEst"] = (la * lv) / 1000.0f;
            }

            JsonArray history = meter["historyKwhByHour"].to<JsonArray>();
            for (uint8_t hour = 0; hour < kHistoryHours; hour++) {
                history.add(simulated_kwh(board, channel, hour));
            }
        }
    }

    String payload;
    serializeJson(doc, payload);
    return payload;
}

static bool set_relay_state(uint8_t board, uint8_t channel, bool state) {
    if (board >= kBoards || channel >= kChannelsPerBoard) {
        Serial.printf("Relay API reject: board=%u channel=%u out of range\n", board, channel);
        return false;
    }
    if (board == 0 && channel < kHardwareRelayChannels) {
        if (!set_i2c_ssr_channel(channel, state)) {
            Serial.printf("Relay API write failed: board=%u channel=%u state=%u\n", board, channel, state ? 1 : 0);
            return false;
        }
    }
    relayShadow[board][channel] = state;
    Serial.printf("Relay API applied: board=%u channel=%u state=%u\n", board, channel, state ? 1 : 0);
    return true;
}

static void setup_webserver() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return;
    }

    for (uint8_t channel = 0; channel < kChannelsPerBoard; channel++) {
        if (channel < kHardwareRelayChannels) {
            relayShadow[0][channel] = get_i2c_ssr_channel(channel);
        } else {
            relayShadow[0][channel] = false;
        }
        relayShadow[1][channel] = false;
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false);
    });

    server.on("/api/dashboard", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", build_dashboard_json());
    });

    server.on("/api/circuitsetup", HTTP_GET, [](AsyncWebServerRequest *request) {
        int house = 0;
        if (request->hasParam("house")) {
            house = request->getParam("house")->value().toInt();
        }
        request->send(200, "application/json", circuitsetup_api_json(house));
    });

    server.on("/api/relay", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("board") || !request->hasParam("channel") || !request->hasParam("state")) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"board,channel,state required\"}");
            return;
        }

        uint8_t board = (uint8_t)request->getParam("board")->value().toInt();
        uint8_t channel = (uint8_t)request->getParam("channel")->value().toInt();
        bool state = request->getParam("state")->value().toInt() != 0;

        if (!set_relay_state(board, channel, state)) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"relay write failed\"}");
            return;
        }

        String response = "{\"ok\":true,\"board\":";
        response += String(board);
        response += ",\"channel\":";
        response += String(channel);
        response += ",\"state\":";
        response += state ? "true" : "false";
        response += ",\"mask\":";
        response += String(get_i2c_ssr_mask());
        response += "}";
        request->send(200, "application/json", response);
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204);
    });

    server.begin();
    Serial.println("Web server started");
}

void setup() {
    Serial.begin(115200);   // Initialize serial communication for debugging
    Serial.println("INFO - Booting...Setup in 3s");
    delay(3000);
    
    // Shared SPI: OLED, MCP2515 CAN, and CircuitSetup ATM90E32 all use SCK/MISO/MOSI (see pins.h).
    // ATM90 library uses global SPI — it must match the PCB routing (not the core default VSPI pins).
    SPI.begin(CIRCUITSETUP_SPI_SCK, CIRCUITSETUP_SPI_MISO, CIRCUITSETUP_SPI_MOSI);
#if defined(CAN0_CS)
    pinMode(CAN0_CS, OUTPUT);
    digitalWrite(CAN0_CS, HIGH);
#endif
#if defined(DISPLAY_CS_PIN)
    pinMode(DISPLAY_CS_PIN, OUTPUT);
    digitalWrite(DISPLAY_CS_PIN, HIGH);
#endif

    generateDeviceID();
    setup_display();
    _console.addLine(" Display up! next is WiFi/Eth, ");
    _console.addLine(" NTP, MQTT, CircuitSetup, Buttons... ");
    // Display startup splash screen (Rick image)
    drawBitmap(40, 5, RICK_WIDTH, RICK_HEIGHT, rick);
    delay(1000);
    
    drawBitmap(0, 0, LOGO_WIDTH, LOGO_HEIGHT, eIOT_logo); // Render Logo
    delay(1000);

    setup_wifi();
    setup_mqtt_client();
    //setup_powerData_caches(); //TODO maybe? openami to have 4-6 subtopics each with a cache planned , needs cache/buffered data for detecting a pattern change 
    //      for optional use each the time its polled or before each time it gets published  - iterate also for  rules and automation
    
#if !HACK_LAB_SKIP_MODBUS
    setup_modbus_master();
    setup_modbus_client();
#else
    Serial.println("HACK_LAB_SKIP_MODBUS: RS-485 Modbus disabled; using CircuitSetup for meter data.");
#endif
    //setup_gpio  // ssr, temp_humid, door contact/tamper. shock, imaging)
    setup_can(); // Initialize CAN bus communication

    setup_buttons();
    setup_i2c_ssr_bank();  // I2C PCF8574 — always on; not gated by HACK_LAB_SKIP_MODBUS (RS-485 only)
    setup_circuitsetup_meter();
    setup_webserver();
    _console.addLine(" EMS In-service Ready!");
    _console.addLine("  CHECK MQTT @");
    _console.addLine("  public.cloud.shiftr.io"); //TODO grab the setup strings from the config file
    _console.addLine("  filter OPENAMI/#");       //TODO grab the setup strings from the config file
    if (wifi_client_connected()) {
        String ipAddress = "  Web UI: http://" + get_wifi_ip();
        _console.addLine(ipAddress.c_str());
        Serial.println(ipAddress);
    }
    _console.addLine("  Push a button?");

}


/**
 * @brief Main program loop that runs continuously
 * 
 * Handles periodic tasks and polling:
 * - Check button inputs
 * - Update display
 * - Process CAN bus messages
 * - Modbus (optional, off in hack lab)
 * - Handle MQTT Publish
 * - Handle MQTT cmd responses 
 * 
 */

unsigned long lastMQTTMillis = 0;
unsigned long lastMQTTPollMillis = 0;


void loop() {
   loop_buttons();
   
#if !HACK_LAB_SKIP_MODBUS
   static unsigned long lastModbusMillis = 0;
   if (millis() - lastModbusMillis > ModbusMaster_pollrate) {
        lastModbusMillis = millis();
        loop_modbus_master();
   }
#endif

   loop_circuitsetup_meter();
   circuitsetup_sync_powerdata_readings();

#if SERIAL_STATUS_INTERVAL_MS > 0
   static unsigned long lastSerialStatusMs = 0;
   {
       const unsigned long nowMs = millis();
       if (nowMs - lastSerialStatusMs >= SERIAL_STATUS_INTERVAL_MS) {
           lastSerialStatusMs = nowMs;
           Serial.printf(
               "STATUS ms=%lu CS_ok=%d/%d CT0=%.2fA CT1=%.2fA r0_I=%.3f WiFi=%s\n",
               nowMs,
               circuitsetup_chip_init_ok(0) ? 1 : 0,
               circuitsetup_chip_init_ok(1) ? 1 : 0,
               circuitsetup_latest_amps(0),
               circuitsetup_latest_amps(1),
               readings[0].current,
               wifi_client_connected() ? "up" : "down");
       }
   }
#endif

   bool poll_due    = (millis() - lastMQTTPollMillis) > (unsigned long)MQTTPoll_rate;
   bool publish_due = (millis() - lastMQTTMillis)     > (unsigned long)MQTTPublish_rootrate;

   if (poll_due || publish_due) {
        maintain_mqtt_connection();

        if (poll_due) {
            lastMQTTPollMillis = millis();
            poll_mqtt();
        }
        if (publish_due) {
            lastMQTTMillis = millis();
            /*
                TODO  calculate which are the normal periodic work items for this loop based on configurable MQTT periodic and adaptive
                PUBLISH operations
                perhaps  12 or so adaptive Publish global bools turned on or off per loop, the periodic  tasks in the subloops can then be targeted to run
                this allows for adaptive rate of openami topics to be published , these adaptive rates can have a defualt periodicity
                but then time of day schedule can chnage the periodicity of the tasks.
                for example publish a base rate of 30 seconds, publish leaks at period of hourly , publish 3 ph summaries and single tenant meters
                 every 15 min itervals, publish harmnics every 15 mins, publish environmentals every 15 mins ,
                 dont publish stuff on same 15 min  cadence (other than the 3Phase and meters must be on hourly edge cadence)  to minmize peak bandwidths
            */
            loop_mqtt();
        }
   }
  
#if !HACK_LAB_SKIP_MODBUS
    loop_modbus_client();
#endif
    loop_buttons(); 
    //loop_display();
    //loop_can();
    //loop_i2c_ssr_bank_serial();
    //loop_i2c_ssr_bank_blink_test();
    // TODO loop_IFTTT();
    // TODO loop_alerts();
    
}