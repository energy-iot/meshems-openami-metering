/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor

   Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina
   Modified to use with EMS Workshop by dmendonca
   Modified to use with draft openami over mqtt by galgie - flexible topic and subtopic reporting of 
        front-of-the-meter StreetPoleEMS and behind-the-meter MDU Building multiEV charge/discharge subpanels
        mediate the resal time energy transfer functions of generate, store, consume, transform, transport actionable edge telemetry for use by both 
        introduced the concept of a distributed LeadEMS  and LVfeeder Lead EMS policy decision serving node
        located within the village, implemented as a Linux Java aggregation node planned for every 50-100 streetpoleEMS and may reside adjacent to a legacy STS/DLMS DCU node. 

   All adaptation GNU General Public License as below.

   -------------------------------------------------------------------

   This file is part of OpenEnergyMonitor.org project.  
   It was further extended by NESL.energy for additional front and behind the meter Sunspec Model report format introducing OPENAMI energy reporting 
   EmonESP is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   EmonESP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with EmonESP; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

Behind the meter Use cases:
    MDU BUILDINg MULTI TENANT BUSS main a LeadEMSs at the MAINs publishes MAINs and balanced energy subsystems to Wan cloud and to Building Lan as mqtt energy status source of truth. 
    other building subsystem EMS subpanels such as HVAC and MultiEV and/or GismoPower stalls or building kitchen appliance subpanels keep each of  their subsystems in profile of Group Lead EMS distributing energy asset transfer policy
    schedules to the various building enrgy subsystems.     
The subsystems EMSs publish to the LeadEMS and optionally to each other on a well known mqtt discovery and negotiation channel
    UPnP like  discovery of Building edges check-in with the designated MAINs LeadEMS to receive each of their policy schedule bulk and periodically iterated updates
    The Lead EMS is a dual ESP32S3 running mostly in hotstandby reporting and remote configurable on an 2 way mqtt OPENAMI to the Utiity cloud edge and i=on a transient as needed can dro sync and allow the standby ESP32 to host a web browser that
    shows energy utlitizations stats and can accepte authenticated local command UX , THe behind the meter architecture is designed as a no single point of failure.  a n:1 EMS sparing strategy is possible in the 
Front of the Meter Use Case
   IEEE ISV StreetPoleEMS multi tenant bidirectional energy asset transfer Policy Enforcer Edge at the Smartened Village StreetPole. S\Each StreetPoleENS communicates with distributed GroupLead EMS policy serving Java Linux Nodes.
   Thes lead Java Linux nodes receive periodic usage telemetry from l\discoverred StreetPoleEMS edges and from front of meter and behind the meter DERs
   that periodically advertise their capabilities and name plate and present utilized capacities - key dimensions of DERs capabilities are  geneerate, transform, store and consume. 
   THe multiple streetpoleems edges  on a shared LVfeeder also has a single (and backup) declared "LVfeeder Lead EMS" that keeps a totalizer data maodel source of truth of the LVfeeder 
   energy transport nameplate capacity and present capacity sharing this over mqtt on a well known  published/discovered mqtt too the GroupLead policcy serving EMS ( Java Linix multicore node. 
   The Java Linux multicore GroupLead EMS policy serving nodes are independant decision makers for one or more designated or learned LVFeeders that it serves the generate, store, transform, and consume time-of-day policies to 

TODO - soon is to breakup this mqtt client as its taking on mqtt higher level separated roles for a designated LVFeeder LeadEMS vs a regular policy enforcing building
   or streetpoleEMS edge of a building energy subsystem specifc policy enforcer or streetpoleEMS LVfeeder EMS policy enforcer role at multitenant subpanel
TODO perhaps can decouple the higher level topics telemetry formating of the key "openami" schema framework separate from the basic emchanical operation of 
establishing and encode and decode json documents and mqtt operations of a 2way mqtt monitor and control plane framework. key openami subtopics
   o  EMS-3phase energy reports actionable telemetry
   o  EMS-harmonics energy actionable telemetry
   o  EMS- leakage energy actioanable telemetry
   o  per tenant meter single phase energy reports with localized leakage actionable telemetry
   o  EMS meaningful stats - actionable OAM learning  telemetry
   o  Tenant per meter stats - for example time stats in active operational edge DER  roles of generate, store, consume, transform
   add ability to add or remove openami subtopics based on the subpanel SKU and onbaord addressable 
   edge sensors (meters, leakage RCM) and actuators (normally closed and normally open designated contactors).
   
   IN a energy equity village and villager  empowerment business model its important to keep a lean well defined 2way mqtt measure and comman and control
   "potential" operations of the edge tenant or streetpole site edge ability to perform in all 4 or 5 modes of addressable 
   session oriented energy asset transfer policy measureable and enforceable energy categories of:
   o consume
   o generate
   o store
   o transform
   o transport

   Ideally each meter should have stats published of its powerflow managed energy asset transfer individual sessions performing as a 
   generate(export), store, consume(import), transform (AC-DC voltage coupled form, voltage level conversion),  transport (as a LVFeeder Lead EMS operational role) managed energy servcice entities  
Summary of New Stats          Counter	Description
mqtt_BWPubOut_payload_bytes
mqtt_BWPubOut_tcpip_bytes
mqtt_publish_count          outbound mqtt published report counts
mqtt_cmd_count++	          Counts total mqtt commands received inbound
mqtt_BWCmdIn_payload_bytes	Sum of all received inbound payload sizes
mqtt_BWCmdIn_tcpip_bytes	  Payload size + 60 bytes per cmd message received inbound
last_bandwidth_report_time  time in secs since last report



   */

#ifdef ENABLE_MQTT
#include "comms/mqtt_client.h"
#include <TimeLib.h>
#ifdef ENABLE_WIFI
  #include <WiFiMulti.h>
#endif
#ifdef ENABLE_ETHERNET
  #include <Ethernet.h>
#endif
#include <core/data_model.h>
#include <core/config.h>
#include <ArduinoJson.h>
#include <metering/modbus_master.h>
#include <metering/sunspec_model_213.h>            // TODO breaks up into base and harmonics separated subtopics for openami
#include <metering/sunspec_model_213_base.h>       // stays true to Sunspec base 213 data model schema
#include <metering/sunspec_model_213_harmonics.h>  // TODO confirm if there is a harmonics report for Sunspec model and adapt or change to be flexible
#include <metering/leakage_model_ivy41a.h>         // these are actioanable leakage sensor measurements based on Type B leakage
#include <metering/sunspec_model_1.h>
#include <hw/i2c_ssr_bank.h>
#include <metering/sunspec_model_11.h>
#include <metering/ems_env_model.h>    
//#include "modbus_devices.h"             // added by Kevin - future use
#include "core/data_model.h"
// Debug MQTT serial output is controlled by the project-wide ENABLE_DEBUG flag.
// Do not add a separate ENABLE_DEBUG_MQTT define here.

// some mqtt banditch stats to include hourly/daily as its own publish
unsigned long mqtt_BWPubOut_payload_bytes = 0;
unsigned long mqtt_BWPubOut_tcpip_bytes = 0;
unsigned long mqtt_BWCmdIn_payload_bytes = 0;
unsigned long mqtt_BWCmdIn_tcpip_bytes = 0;
unsigned long mqtt_publish_count = 0; // outbound published report counts
unsigned long mqtt_cmd_count = 0;     // inbound  recived CMC request counts 
unsigned long last_bandwidth_report_time = 0;
//const unsigned long BANDWIDTH_REPORT_INTERVAL_MS = 3600000; // 1 hour report interval on mqtt bandwidth stats per subpanel
const unsigned long BANDWIDTH_REPORT_INTERVAL_MS = 300000; // debug only 5 min report interval on mqtt bandwidth stats per subpanel


#ifdef ENABLE_ETHERNET
  EthernetClient transportClient;
#else
  WiFiClient transportClient;
#endif
PubSubClient mqttclient(transportClient);

// TODO is to allow build time control bools here to enable openami schema subtopics to be included or not based on a subpanel model - TBD

unsigned long mqtt_interval_ts = 0;
static char mqtt_data[128] = "";
static int mqtt_connection_error_count = 0;
String topic_device;          // StreetPoleEMS globally unique device topic (publish/subscribe under here)
String topic_cmd;             // command topic (for 'southbound' commands)

// Function prototype for mqtt_publish_json
void mqtt_publish_json(const char* subtopic, const JsonDocument* payload);

void generateTopics() {
  // Generate device topic: MQTT_TOPIC/device_id/
  topic_device.reserve(strlen(MQTT_TOPIC) + 1 + strlen(getDeviceID()) + 1 + 16);
  topic_device = MQTT_TOPIC;
  topic_device.concat("/");
  topic_device.concat(getDeviceID());
  topic_device.concat("/");

  // Generate command topic: MQTT_TOPIC/device_id/cmd
  topic_cmd = topic_device;
  topic_cmd.concat("cmd");
}

// -------------------------------------------------------------------
// MQTT Connect
// Called only when MQTT server field is populated
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  Serial.printf("MQTT Connecting...timeout in:%d\r\n", transportClient.getTimeout());
  // todo ENABLE_DEBUG_MQTT=1;  // allow for 1883 or 8883 encrypted telemetry and command and control
  if (transportClient.connect(MQTT_SERVER, 1883) != 1) //8883 for TLS
  {
     Serial.println("MQTT connect timeout.");
     // todo ENABLE_DEBUG_MQTT=0;
     return (0);
  }

  //transportClient.setTimeout(60);//(MQTT_TIMEOUT);
  mqttclient.setSocketTimeout(6);//MQTT_TIMEOUT);
  mqttclient.setBufferSize(MAX_DATA_LEN + 200);
  mqttclient.setKeepAlive(180);

  if (strcmp(MQTT_USER, "") == 0) {
    //allows for anonymous connection
    mqttclient.connect(getDeviceID()); // Attempt to connect
  } else {
    mqttclient.connect(getDeviceID(), MQTT_USER, MQTT_PW); // Attempt to connect
  }

  if (mqttclient.state() == 0) {
    Serial.printf("MQTT connected: %s\r\n", MQTT_SERVER);
    
    //subscribe to command topic
    if (!mqttclient.subscribe(topic_cmd.c_str())) {
      delay(250);
      if (!mqttclient.subscribe(topic_cmd.c_str())) {
        delay(500);
        if (!mqttclient.subscribe(topic_cmd.c_str())) {
          Serial.printf("MQTT: FAILED TO SUBSCRIBE TO COMMAND TOPIC: %s\r\n", topic_cmd.c_str());
          return false;
        }
      }
    }
    Serial.printf("MQTT: SUBSCRIBED TO COMMAND TOPIC: %s\r\n", topic_cmd.c_str());
    //  mqttclient.publish(getDeviceTopic().c_str(), "connected"); // Once connected, publish an announcement..
  } else {
    Serial.println("MQTT failed: ");
    Serial.println(mqttclient.state());
    return (0);
  }
  return (1);
}

/**
 * Publish a SunSpec Model 213 (3-phase AC meter) report aggregated from all
 * available CT channels.
 *
 * Channel-to-phase mapping (IC1: channels 0-2, IC2: channels 3-5):
 *   ch 0, 3 → Phase A
 *   ch 1, 4 → Phase B
 *   ch 2, 5 → Phase C
 *
 * Current, active power, reactive power, apparent power, and energy are summed
 * per phase.  Voltage is taken from the first channel assigned to each phase
 * (IC1 channel for that phase).  Power factor per phase is derived from
 * active / apparent power (guarded against division by zero).
 *
 * Accepts up to 6 channels; extra channels beyond 6 are ignored.
 */
void mqtt_publish_EMS_3Ph(const PowerData readings[], int count) {
  // Channel-to-phase index: 0=A, 1=B, 2=C (IC1 ch 0/1/2 and IC2 ch 3/4/5
  // map to the same three phases, so two CT channels contribute per phase).
  static const int CHANNEL_TO_PHASE[6] = {0, 1, 2, 0, 1, 2};

  float voltage[3]        = {0.0f};
  float current[3]        = {0.0f};
  float active_power[3]   = {0.0f};
  float reactive_power[3] = {0.0f};
  float apparent_power[3] = {0.0f};
  float import_energy[3]  = {0.0f};
  float export_energy[3]  = {0.0f};
  bool  volt_set[3]       = {false, false, false};
  float frequency         = 0.0f;
  long  timestamp         = 0;

  int effective = (count < 6) ? count : 6;
  for (int ch = 0; ch < effective; ch++) {
    int ph = CHANNEL_TO_PHASE[ch];
    // Use the first (IC1) channel's voltage and frequency for each phase.
    if (!volt_set[ph]) {
      voltage[ph] = readings[ch].voltage;
      volt_set[ph] = true;
    }
    if (frequency == 0.0f) frequency = readings[ch].frequency;
    if (timestamp == 0)    timestamp  = (long)readings[ch].timestamp_last_report;
    current[ph]        += readings[ch].current;
    active_power[ph]   += readings[ch].active_power;
    reactive_power[ph] += readings[ch].reactive_power;
    apparent_power[ph] += readings[ch].apparent_power;
    import_energy[ph]  += readings[ch].import_energy;
    export_energy[ph]  += readings[ch].export_energy;
  }

  SunSpecModel213 sunSpecData;
  // Phase A
  sunSpecData.PhVphA  = voltage[0];
  sunSpecData.AphA    = current[0];
  sunSpecData.WphA    = active_power[0]   * 1000.0f;
  sunSpecData.VarphA  = reactive_power[0] * 1000.0f;
  sunSpecData.VAphA   = apparent_power[0] * 1000.0f;
  sunSpecData.PFphA   = (apparent_power[0] > 0.0f)
                          ? active_power[0] / apparent_power[0] : 0.0f;
  sunSpecData.TotWhAImport = import_energy[0] * 1000.0f;
  sunSpecData.TotWhAExport = export_energy[0] * 1000.0f;
  // Phase B
  sunSpecData.PhVphB  = voltage[1];
  sunSpecData.AphB    = current[1];
  sunSpecData.WphB    = active_power[1]   * 1000.0f;
  sunSpecData.VarphB  = reactive_power[1] * 1000.0f;
  sunSpecData.VAphB   = apparent_power[1] * 1000.0f;
  sunSpecData.PFphB   = (apparent_power[1] > 0.0f)
                          ? active_power[1] / apparent_power[1] : 0.0f;
  sunSpecData.TotWhBImport = import_energy[1] * 1000.0f;
  sunSpecData.TotWhBExport = export_energy[1] * 1000.0f;
  // Phase C
  sunSpecData.PhVphC  = voltage[2];
  sunSpecData.AphC    = current[2];
  sunSpecData.WphC    = active_power[2]   * 1000.0f;
  sunSpecData.VarphC  = reactive_power[2] * 1000.0f;
  sunSpecData.VAphC   = apparent_power[2] * 1000.0f;
  sunSpecData.PFphC   = (apparent_power[2] > 0.0f)
                          ? active_power[2] / apparent_power[2] : 0.0f;
  sunSpecData.TotWhCImport = import_energy[2] * 1000.0f;
  sunSpecData.TotWhCExport = export_energy[2] * 1000.0f;
  // Totals across all phases (Wh)
  sunSpecData.TotWhImport = (import_energy[0] + import_energy[1] + import_energy[2]) * 1000.0f;
  sunSpecData.TotWhExport = (export_energy[0] + export_energy[1] + export_energy[2]) * 1000.0f;
  sunSpecData.Hz = frequency;

  JsonDocument jsonDoc;
  sunSpecData.toJson(jsonDoc);
  jsonDoc["timestamp"] = timestamp;

  mqtt_publish_json("subpanel_3Ph", &jsonDoc);
}

void mqtt_publish_Meter(int meterId, const PowerData& meterData) { 
  SunSpecModel11 sunSpecData;
  // publish Model 11  SUnspec schema here for per tenant single phase
  // For now assume phase A. This can be extended to put the meter readings in the
  // correct phase using configuration data about which meter is on which phase.
  //TODO grab the specific cached meterId  PowerData[i] for example
  sunSpecData.Phase= meterId; // assume 1 tenenat per phase in a 3 tenant 3ph subpanel , TODO part of stage operation and subpanel schema backed up  to a subpanel staging cloud 
  sunSpecData.PhV= meterData.voltage;
  sunSpecData.PhA = meterData.current;
  sunSpecData.PhW = meterData.active_power * 1000;
  sunSpecData.TotWhImport = meterData.import_energy * 1000;
  sunSpecData.TotWhExport = meterData.export_energy * 1000;
  sunSpecData.Hz = meterData.frequency;
  sunSpecData.PF  = meterData.power_factor;
  // Assign apparent and reactive power to their correct SunSpec fields.
  // See data_model.h unit convention: fields stored in kVA/kVAr, * 1000 → VA/VAr.
  sunSpecData.VA  = meterData.apparent_power  * 1000;
  sunSpecData.Var = meterData.reactive_power  * 1000;
// TODO add actioanable locally interpreted metadata to the report 

  long timestamp = meterData.timestamp_last_report;
  // Use stack buffer to avoid String heap allocation (consistent with mqtt_publish_json).
  char topicBuf[16];
  snprintf(topicBuf, sizeof(topicBuf), "meter_%d", meterId);

  JsonDocument jsonDoc;
  sunSpecData.toJson(jsonDoc);
  jsonDoc["timestamp"] = timestamp;

  mqtt_publish_json(topicBuf, &jsonDoc);
}

// Publish SunSpec Model 1 manufacturer/nameplate data for this subpanel.
void mqtt_publish_EMS_MFR(long timestamp) {
  SunSpecModel1_EMS MFRData;
  JsonDocument jsonDoc;
  MFRData.toJson(jsonDoc);
  jsonDoc["timestamp"] = timestamp;
  mqtt_publish_json("subpanel_MFR", &jsonDoc);
}

// Publish subpanel environmental data — temperature and humidity from the SHT20
// sensor (always present on the RS-485 bus).
void mqtt_publish_EMS_ENV(long timestamp) {
  EMS_ENV_Model EMS_ENV_cache;
  EMS_ENV_cache.timestamp_ms = (unsigned long)timestamp;
#ifdef ENABLE_MODBUS_MASTER
  // Only populate sensor values once the SHT20 has returned at least one
  // successful poll response — avoids publishing 0°C / 0% as real data
  // during boot or when the sensor is absent.
  if (get_sht20_success_count() > 0) {
    EMS_ENV_cache.temperature_C    = get_sht20_temperature();
    EMS_ENV_cache.humidity_percent = get_sht20_humidity();
    EMS_ENV_cache.last_modbus_update_ms = millis();
  }
#endif
  JsonDocument jsonDoc;
  EMS_ENV_cache.toJson(jsonDoc);
  jsonDoc["timestamp"] = timestamp;
  mqtt_publish_json("subpanel_ENV", &jsonDoc);
}
void mqtt_publish_Harmonics(long timestamp) { // TODO pass EMSdata structured model
  String topicBuf = "subpanel_harmonics"; // Subtopic under the streetPoleEMS per unique nodal topic
  SunSpecModel213Harmonics harmonicsData;
  JsonDocument jsonDoc;
  harmonicsData.toJson(jsonDoc);
  jsonDoc["timestamp"] = timestamp;
  mqtt_publish_json("subpanel_harmonics", &jsonDoc);
}

void mqtt_publish_Leakage(String meterId, const PowerData& meterData) {
  LeakageModel leakageData;

  // TODO prepare actionable DC and AC leakage measurments, patterns and stats, and faults, and outages
  // TODO add adaptive publish rate as leakage grows from none to 
  // TODO see modbus register suite from IVY Metering RCD, RVD, differentiator for AC leakage is to include phase angle of leakage current vs phase 
  // leakage can be powerflow direction sensitive  and dependant
  long timestamp = meterData.timestamp_last_report;
  String topicBuf = "subpanel_RCMleaks"; //TODO + phaseId; and/OR + meterid;

  JsonDocument jsonDoc;
  leakageData.toJson(jsonDoc);
  jsonDoc["timestamp"] = timestamp;

  mqtt_publish_json(topicBuf.c_str(), &jsonDoc);
}

/*
MQTT Stats
What We’ll Track and report hourly
  MQTT payload bytes (JSON body).
  Estimated TCP/IP overhead per publish (default assumption: ~60 bytes per publish).
  Packet count.


void mqtt_publish_BWPubOut_stats() {  // outgoing bandwidth used per streetpoleEMS stat
    JsonDocument statsDoc;
    statsDoc["interval_ms"] = BANDWIDTH_REPORT_INTERVAL_MS;
    statsDoc["pubout_count"] = mqtt_publish_count;
    statsDoc["payload_bytes"] = mqtt_BWPubOut_payload_bytes;
    statsDoc["tcpip_bytes"] = mqtt_BWPubOut_tcpip_bytes;
    statsDoc["timestamp"] = now();

    mqtt_publish_json("subpanel_BWPubOut", &statsDoc);

    // Reset counters
    mqtt_BWPubOut_payload_bytes = 0;
    mqtt_BWPubOut_tcpip_bytes = 0;
    mqtt_publish_count = 0;
}
*/

void mqtt_publish_BWPubOut_stats() {
    JsonDocument statsDoc;

    unsigned long current_time = millis();
    unsigned long elapsed_ms = current_time - last_bandwidth_report_time;
    float elapsed_sec = elapsed_ms / 1000.0;

    // Calculate average bits per second
    unsigned long total_bits_out = mqtt_BWPubOut_tcpip_bytes * 8;
    float avg_bps_out = (elapsed_sec > 0) ? total_bits_out / elapsed_sec : 0;

    statsDoc["interval_ms"] = BANDWIDTH_REPORT_INTERVAL_MS;
    statsDoc["pubout_count"] = mqtt_publish_count;
    statsDoc["payload_bytes"] = mqtt_BWPubOut_payload_bytes;
    statsDoc["tcpip_bytes"] = mqtt_BWPubOut_tcpip_bytes;
    statsDoc["avg_bps_out"] = (int)avg_bps_out;
    statsDoc["timestamp"] = now();

    mqtt_publish_json("subpanel_BWPubOut", &statsDoc);

    // Reset counters
    mqtt_BWPubOut_payload_bytes = 0;
    mqtt_BWPubOut_tcpip_bytes = 0;
    mqtt_publish_count = 0;
    last_bandwidth_report_time = current_time;
}

void mqtt_publish_BWCmdIn_stats() { // incoming bandwidth used per streetpoleEMS stat
    JsonDocument statsDoc;

    unsigned long current_time = millis();
    unsigned long elapsed_ms = current_time - last_bandwidth_report_time;
    float elapsed_sec = elapsed_ms / 1000.0;

    // Calculate average bits per second (bps) for incoming traffic
    unsigned long total_bits_in = mqtt_BWCmdIn_tcpip_bytes * 8;
    float avg_bps_in = (elapsed_sec > 0) ? total_bits_in / elapsed_sec : 0;

    statsDoc["interval_ms"] = BANDWIDTH_REPORT_INTERVAL_MS;
    statsDoc["cmdin_count"] = mqtt_cmd_count;
    statsDoc["payload_bytes"] = mqtt_BWCmdIn_payload_bytes;
    statsDoc["tcpip_bytes"] = mqtt_BWCmdIn_tcpip_bytes;
    statsDoc["avg_bps_in"] = (int)avg_bps_in;
    statsDoc["timestamp"] = now();

    mqtt_publish_json("subpanel_BWCmdIn", &statsDoc);

    // Reset counters
    mqtt_BWCmdIn_payload_bytes = 0;
    mqtt_BWCmdIn_tcpip_bytes = 0;
    mqtt_cmd_count = 0;
}


// Method to publish json blob in a mqtt subtopic
void mqtt_publish_json(const char* subtopic, const JsonDocument* payload) {
    // Use stack buffers where possible to reduce heap fragmentation from String objects
    size_t payload_len = measureJson(*payload);
    if (payload_len >= 1024) {
        Serial.println("MQTT publish: payload too large");
        return;
    }

    // Serialize JSON directly to char buffer - avoids intermediate String allocation
    char data[1024];
    serializeJson(*payload, data, sizeof(data));
    
    // Build topic on stack buffer - avoid String concatenation for reduced heap usage
    char topicBuf[256];
    int topic_len = snprintf(topicBuf, sizeof(topicBuf), "%s%s",
                             topic_device.c_str(), subtopic);

    if (topic_len < 0 || topic_len >= (int)sizeof(topicBuf)) {
        Serial.println("MQTT publish: topic buffer overflow");
        return;
    }

    if (!mqttclient.publish(topicBuf, data)) {
        Serial.println("MQTT publish: failed");
    } else { // update stats on mqtt bandwidth used per streetpoleEMS
        mqtt_BWPubOut_payload_bytes += payload_len;
        mqtt_BWPubOut_tcpip_bytes += payload_len + 60; // TCP/IP+MQTT overhead
        mqtt_publish_count++;
    }

#ifdef ENABLE_DEBUG
    Serial.printf("topic: %s, data: %s\n", topicBuf, data);
#endif
}


//pull apart a comma-sep colon-delim name:value string and publish the name:value pairs under 'subtopic'
void mqtt_publish_comma_sep_colon_delim(const char* subtopic, const char * data) {
    char topicBuf[256];
    char buf[256];
    Serial.printf("MQTT publish: size:%d chars", strlen(data));
    do {
      int pos = strcspn(data, ":");
      strncpy(buf, data, pos);
      buf[pos] = 0;
      int topic_len = snprintf(topicBuf, sizeof(topicBuf), "%s%s/%s",
                               topic_device.c_str(), subtopic, buf);
      //topic_ptr[pos] = 0;
      data += pos;
      if (*data++ == 0) {
        break;
      }

      pos = strcspn(data, ",");
      strncpy(mqtt_data, data, pos);
      mqtt_data[pos] = 0;
      data += pos;

      if (!mqttclient.publish(topicBuf, mqtt_data)) {
       Serial.println("MQTT publish: failed");
      }
#ifdef ENABLE_DEBUG
      Serial.printf("topic: %s, data: %s\n", topicBuf, mqtt_data);
#endif
    } while (*data++ != 0);
}

// -------------------------------------------------------------------
// Command handlers — one function per command keyword.
// Each receives the parsed JSON document so it can extract parameters.
// -------------------------------------------------------------------
static void cmd_report  (const JsonDocument&) { Serial.printf("matched \"report\"\n");   }
static void cmd_meter   (const JsonDocument&) { Serial.printf("matched \"meter\"\n");    }
static void cmd_bms     (const JsonDocument&) { Serial.printf("matched \"bms\"\n");      }
static void cmd_inverter(const JsonDocument&) { Serial.printf("matched \"inverter\"\n"); }

#ifdef ENABLE_RELAYS
/**
 * Example relay payloads:
 * {"cmd":"relay","address":0,"kwh_limit":100.0}
 * {"cmd":"relay","address":3,"kw_limit":5.5}
 * {"cmd":"relay","address":7,"kwh_limit":250.0,"kw_limit":12.0}
 * {"cmd":"relay","address":3,"kwh_limit":80.0}
 * {"cmd":"relay","address":2,"state":"open"}
 */
static void cmd_relay(const JsonDocument& doc) {
    if (!doc["address"].is<int>()) {
        Serial.println("relay cmd: missing or invalid 'address'");
        return;
    }
    int address = doc["address"].as<int>();
    if (address < 0 || address > 7) {
        Serial.printf("relay cmd: address %d out of range 0-7\n", address);
        return;
    }

    bool has_state = doc["state"].is<const char*>();
    bool has_kwh   = doc["kwh_limit"].is<float>() || doc["kwh_limit"].is<int>();
    bool has_kw    = doc["kw_limit"].is<float>()  || doc["kw_limit"].is<int>();

    if (has_state && (has_kwh || has_kw)) {
        Serial.println("relay cmd: 'state' is mutually exclusive with limit fields");
        return;
    }
    if (!has_state && !has_kwh && !has_kw) {
        Serial.println("relay cmd: must provide 'state', 'kwh_limit', or 'kw_limit'");
        return;
    }

    if (has_state) {
        // TODO: call set_ssr_channel() once implemented in i2c_ssr_bank
        Serial.printf("relay cmd: ch %d state='%s' received (actuation not yet wired)\n",
                      address, doc["state"].as<const char*>());
        return;
    }

    RelayRule rule;
    rule.kwh_limit = has_kwh ? doc["kwh_limit"].as<float>() : -1.0f;
    rule.kw_limit  = has_kw  ? doc["kw_limit"].as<float>()  : -1.0f;
    set_relay_rule((uint8_t)address, rule);

    Serial.println("relay rules:");
    for (uint8_t ch = 0; ch < 8; ch++) {
        RelayRule r = get_relay_rule(ch);
        Serial.printf("  ch %d: kwh_limit=%.2f kw_limit=%.2f\n", ch, r.kwh_limit, r.kw_limit);
    }
}
#endif // ENABLE_RELAYS

// Dispatch table — add new {keyword, handler} rows here to extend.
typedef void (*cmd_handler_t)(const JsonDocument&);
struct CmdEntry { const char* keyword; cmd_handler_t handler; };
static const CmdEntry cmd_table[] = {
  { "report",   cmd_report   },
  { "meter",    cmd_meter    },
  { "bms",      cmd_bms      },
  { "inverter", cmd_inverter },
#ifdef ENABLE_RELAYS
  { "relay",    cmd_relay    },
#endif
};

// Subscriber callback
//
// We're subscribed to the following topics:
// <top>/<device_id>/cmd
//
//TODO add a few lines where the mqtt_cmd_count stat is incremented
// 
void subscriber_callback(char* topic, uint8_t* payload, unsigned int length) {
  //sanity
  if (length > 254) {
    Serial.printf("MQTT CALLBACK: not handled: payload len overrun:%d\n", length);
    return;
  }

  if (strcmp(topic, topic_cmd.c_str()) == 0) {
    mqtt_cmd_count++;                   // <-- stat to INCREMENT Cmd inbound COUNT HERE
    mqtt_BWCmdIn_payload_bytes += length;            // Count the raw payload size
    mqtt_BWCmdIn_tcpip_bytes += length + 60;         // Add estimated MQTT+TCP/IP overhead
    char payload_buf[length+1] = {0};
    strncpy(payload_buf, (char*)payload, length);
    payload_buf[length] = '\0';         //ensure null-termination
    Serial.printf("\n***MQTT CALLBACK: topic '%s', payload '%s'\n", topic, payload_buf);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload_buf);
    if (err) {
      Serial.printf("MQTT CALLBACK: JSON parse error: %s\n", err.c_str());
      return;
    }
    const char* cmd = doc["cmd"] | "";
    bool matched = false;
    for (size_t i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++) {
      if (strcmp(cmd, cmd_table[i].keyword) == 0) {
        cmd_table[i].handler(doc);
        matched = true;
        break;
      }
    }
    if (!matched) {
      Serial.printf("MQTT CALLBACK: no match for cmd '%s'\n", cmd);
    }
  }
}

void setup_mqtt_client() {
  generateTopics();
  mqttclient.setCallback(subscriber_callback);
  if (!mqtt_connect()) {
    delay(250);
    if (!mqtt_connect()) {
      delay(500);
      if (!mqtt_connect()) {
        Serial.println("MQTT: FAILED TO CONNECT");
        return;
      }
    }
  }
  mqtt_interval_ts = now();
}

void loop_mqtt() {
  uint32_t loop_timestamp = esp_log_timestamp();

      //mqtt_publish(input);
      if (mqttclient.connected()) {  
      // TODO not all telemetry has to publish on same loop iteration, different rates of publish , 
      // target requirement is publish on a adaptive meaningful rate is a lean bandwidth on the meshed G3/PLC/Wireless/LORA MAC/PHY future iteration

       // First is to  publish Sunspec model 1 subpanel manufacture details TODO is 1 per day or per hour
      mqtt_publish_EMS_MFR(loop_timestamp);  //publish Sunspec model 1 manufacture details
      #ifdef ENABLE_DEBUG
      Serial.println("Published EMS Model 1 MFR Info");
      #endif

      // publish real time subpanel cabinet environmetals such as temp/pressure/humid/  tamper and 
      // TODO integrate shock and  loud noise and possily street pole image snapsot on demand from mqtt command
      mqtt_publish_EMS_ENV(loop_timestamp);
      #ifdef ENABLE_DEBUG
      Serial.println("Published subpanel environmental data");
      #endif

    // TODO Next publish Sunspec model xyz subpanel DER nameplate capacity  rating
      // mqtt_publish_EMS_Rated(readings[0]);  //publish Sunspec model xyza rating details
      // Serial.println("Published EMS nameplate");
       //TODO publish 1 or 3 phase OPENAMI per subpanel peer phase and per meter/tenant energy usage (TODO scope is consumed, generated, stored, transformed, distributed);
        //TODO  IF 3phase phase subpanel setup then - assume 3 phase subpanel 
#ifdef ENABLE_MODBUS_MASTER
      mqtt_publish_EMS_3Ph(readings, MODBUS_NUM_METERS);  // publish Sunspec model 213 schema for the 3 phase subpanel
      // TODO else publish single phase subpanel totalizer metrics
      //  mqtt_publish_EMS_1Ph(readings[0]);  // publish Sunspec model 213 schema for the 1 phase subpanel
      #ifdef ENABLE_DEBUG
       Serial.println("Published EMS per Phase Totalizers");
      #endif
       //next is publish EMS per phase Leakage , TODO adpative rate: if leakage is non zero or leakage fault or leakage changed
      mqtt_publish_Leakage("", readings[0]);
      #ifdef ENABLE_DEBUG
      Serial.println("Published per phase leakage");
      #endif

      //next is publish EMS per phase Harmonics , TODO adpative rate: if leakage is non zero or leakage fault or leakage changed
      mqtt_publish_Harmonics(loop_timestamp);
      #ifdef ENABLE_DEBUG
      Serial.println("Published per phase Harmonics");
      #endif

    // next is loop over the subpanel per tenant meters   
    for(int i=0;i<MODBUS_NUM_METERS;i++) {
          mqtt_publish_Meter(i, readings[i]);  
          // TODO add modbus node number and per meter leakage RCD Fault in the readings powerdata
          #ifdef ENABLE_DEBUG
          Serial.printf("Published tenant meter %d\n", i);
          #endif
        }
#endif // ENABLE_MODBUS_MASTER
      } else {
        Serial.println("MQTT not connected!");
      }
      mqtt_interval_ts = millis();
    if (millis() - last_bandwidth_report_time >= BANDWIDTH_REPORT_INTERVAL_MS) {
     mqtt_publish_BWPubOut_stats();
     mqtt_publish_BWCmdIn_stats();
     // Note: last_bandwidth_report_time is already reset inside
     // mqtt_publish_BWPubOut_stats(); do not reset it again here.
     #ifdef ENABLE_DEBUG
     Serial.println("Published stats/bandwidth");
     #endif
    }
}

void poll_mqtt() {
    mqttclient.loop();
}

void maintain_mqtt_connection() {
    if (!mqttclient.connected()) {
        Serial.println("MQTT: connection lost, attempting reconnect");
        mqtt_connect();
    }
}

void mqtt_restart()
{
  if (mqttclient.connected()) {
    mqttclient.disconnect();
  }
}

boolean mqtt_connected()
{
  return mqttclient.connected();
}


// TODO add door contact/tamper and publish in OPENAMI EMS nde subtopic
void mqtt_publish_door_opened() {
  // Buffer sized to hold topic_device (~40 chars) + "/door" + null terminator.
  char buf[128] = {0};
  snprintf(buf, sizeof(buf), "%s/door", topic_device.c_str());
  mqttclient.publish(buf, "open", 0);
}

void mqtt_publish_door_closed() {
  char buf[128] = {0};
  snprintf(buf, sizeof(buf), "%s/door", topic_device.c_str());
  mqttclient.publish(buf, "closed", 0);
}

#endif // ENABLE_MQTT
