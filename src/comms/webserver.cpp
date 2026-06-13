#ifdef ENABLE_WEBSERVER

#if !defined(ENABLE_WIFI) && !defined(ENABLE_ETHERNET)
  #error "ENABLE_WEBSERVER requires ENABLE_WIFI or ENABLE_ETHERNET."
#endif

#include "comms/webserver.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <core/config.h>
#include <core/data_model.h>

#ifdef METER_TYPE_ATM90E32
  #include "circuitsetup_meter.h"
#endif

#ifdef ENABLE_RELAYS
  #include <hw/i2c_ssr_bank.h>
#endif

static AsyncWebServer s_server(80);

// ---------------------------------------------------------------------------
// /api/dashboard
// ---------------------------------------------------------------------------

static String build_dashboard_json() {
    JsonDocument doc;

    doc["siteId"]      = getDeviceID();
    doc["uptimeMs"]    = millis();

#ifdef METER_TYPE_ATM90E32
    doc["meterSource"] = "ATM90E32";
    JsonArray chipsOk = doc["circuitSetupChipsOk"].to<JsonArray>();
    chipsOk.add(circuitsetup_chip_init_ok(0));
    chipsOk.add(circuitsetup_chip_init_ok(1));
#else
    doc["meterSource"] = "Modbus";
    doc["circuitSetupChipsOk"].to<JsonArray>();
#endif

    // Village centre — override by placing a /data/site-config.json on SPIFFS
    // if you want the map to open on a real site. Defaults to Kampala area.
    JsonObject center = doc["villageCenter"].to<JsonObject>();
    center["lat"]  = 0.3476;
    center["lon"]  = 32.5825;
    center["zoom"] = 14;

    // GeoJSON overlay — empty by default; upload a features array via SPIFFS
    // to show pole/line/court assets on the map.
    JsonObject geo = doc["utilityGeoJson"].to<JsonObject>();
    geo["type"] = "FeatureCollection";
    geo["features"].to<JsonArray>();

    // Boards → meters
    JsonArray boards = doc["boards"].to<JsonArray>();
    JsonObject board = boards.add<JsonObject>();
    board["boardId"] = 1;
    JsonArray meters = board["meters"].to<JsonArray>();

    for (int i = 0; i < MODBUS_NUM_METERS; i++) {
        JsonObject m = meters.add<JsonObject>();
        m["meterId"]      = i;
        m["relayChannel"] = i;

        char label[20];
        snprintf(label, sizeof(label), "Tenant %d", i + 1);
        m["tenantLabel"] = label;

        int ctCh = i % 6;
        char ctLabel[8];
        snprintf(ctLabel, sizeof(ctLabel), "CT%d", ctCh + 1);
        m["ctLabel"]   = ctLabel;
        m["ctChannel"] = ctCh;

#ifdef ENABLE_RELAYS
        m["relayState"] = get_i2c_ssr_channel((uint8_t)i);
#else
        m["relayState"] = false;
#endif

#ifdef METER_TYPE_ATM90E32
        float amps  = circuitsetup_latest_amps(ctCh);
        float volts = circuitsetup_phase_voltage(ctCh);
        m["liveAmps"]    = amps;
        m["liveVolts"]   = volts;
        m["liveKwEst"]   = (amps * volts) / 1000.0f;
        m["dailyKwhTotal"] = readings[i].import_energy;
#else
        m["liveAmps"]    = readings[i].current;
        m["liveVolts"]   = readings[i].voltage;
        m["liveKwEst"]   = readings[i].active_power;
        m["dailyKwhTotal"] = readings[i].import_energy;
#endif
    }

    String out;
    serializeJson(doc, out);
    return out;
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------

void setup_webserver() {
    if (!SPIFFS.begin(true)) {
        Serial.println("WebServer: SPIFFS mount failed — web UI unavailable");
        return;
    }

    // Static files from SPIFFS (data/ folder: index.html, mapbox-config.js, etc.)
    s_server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // GET /api/dashboard
    s_server.on("/api/dashboard", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", build_dashboard_json());
    });

    // GET /api/circuitsetup?house=N
    s_server.on("/api/circuitsetup", HTTP_GET, [](AsyncWebServerRequest* req) {
#ifdef METER_TYPE_ATM90E32
        int house = req->hasParam("house") ? req->getParam("house")->value().toInt() : 0;
        req->send(200, "application/json", circuitsetup_api_json(house));
#else
        req->send(503, "application/json", "{\"error\":\"ATM90E32 meter not enabled\"}");
#endif
    });

    // GET /api/relay?board=N&channel=M&state=0|1
    s_server.on("/api/relay", HTTP_GET, [](AsyncWebServerRequest* req) {
#ifdef ENABLE_RELAYS
        if (!req->hasParam("channel") || !req->hasParam("state")) {
            req->send(400, "application/json", "{\"error\":\"channel and state params required\"}");
            return;
        }
        int  channel = req->getParam("channel")->value().toInt();
        bool state   = req->getParam("state")->value().toInt() != 0;
        int  board   = req->hasParam("board") ? req->getParam("board")->value().toInt() : 1;

        if (channel < 0 || channel > 7) {
            req->send(400, "application/json", "{\"error\":\"channel must be 0-7\"}");
            return;
        }

        bool ok = set_i2c_ssr_channel((uint8_t)channel, state);

        JsonDocument doc;
        doc["ok"]      = ok;
        doc["board"]   = board;
        doc["channel"] = channel;
        doc["state"]   = state;
        doc["mask"]    = get_i2c_ssr_mask();

        String out;
        serializeJson(doc, out);
        req->send(ok ? 200 : 500, "application/json", out);
#else
        req->send(503, "application/json", "{\"error\":\"ENABLE_RELAYS not active\"}");
#endif
    });

    s_server.begin();
    Serial.println("WebServer: started on port 80");
}

#endif // ENABLE_WEBSERVER
