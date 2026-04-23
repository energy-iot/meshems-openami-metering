#pragma once
#include <Arduino.h>

#define MAX_DEVICE_ID_CHARS   32
#define DEVICE_ID_PREFIX      "StreetPoleEMS_"

#define MQTT_TOPIC              "openami" // "openami/StreetPoleEMS_<EMSid>"
//#define MQTT_TOPIC              "nesl"
#define MQTT_PUBLISH_INTERVAL   30000
#define MQTT_SERVER             "public.cloud.shiftr.io"  //"test.mosquitto.org"
#define MQTT_USER               "public"                  // leave empty for test.mosquitto.org
#define MQTT_PW                 "public"                  // leave empty for test.mosquitto.org   

// DTM485 custom Ascii on serial on rsa485 -  Serial Pins and Baud Rate
#define DTM485_SERIAL Serial2
#define DTM485_DE_RE_PIN 23
#define DTM485_BAUDRATE 9600

// Poll and Publish Intervals
#define DTM485POLL_INTERVAL_MS 5000        // 5 seconds
#define DTM_MQTT_PUBLISH_INTERVAL_MS 300000   // 5 minutes

// 1 = skip all RS-485 Modbus (master + client): no DDS238/SHT20 polling, no Modbus UART init.
// Meter UI + MQTT use CircuitSetup SPI meters. Set to 0 when RS485 tenant meters / SHT20 return.
// I2C is unchanged: PCF8574 SSR bank (tenant controls) still runs — see setup_i2c_ssr_bank() in main.cpp.
#define HACK_LAB_SKIP_MODBUS 1

// When Modbus master is enabled: poll SHT20 temp/humidity on RS485 (addr 1). Default off — no sensor /
// known timeout issues spam the serial log; set to 1 only with a working SHT20 on UART1 RS485.
#define MODBUS_ENABLE_SHT20 0

/** USB serial one-line heartbeat (ms). Independent of Modbus; set 0 to disable STATUS prints. */
#define SERIAL_STATUS_INTERVAL_MS 3000u

//TODO allow these to 
extern int ModbusMaster_pollrate;    //in 1000's millisecond or seconds
extern int MQTTPublish_rootrate;  //in 1000's millisecond or seconds
extern int MQTTPoll_rate;         //in milliseconds, how often to call mqttclient.loop()



void generateDeviceID();
const char* getDeviceID();