#ifdef ENABLE_ETHERNET
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <core/pins.h>
#include <comms/ethernet.h>

// Locally-administered MAC — change the last three octets per device if
// multiple boards share the same network segment.
static byte eth_mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

bool ethernet_connected() {
    return Ethernet.linkStatus() == LinkON && Ethernet.localIP() != IPAddress(0, 0, 0, 0);
}

String get_eth_ip() {
    if (ethernet_connected()) {
        return Ethernet.localIP().toString();
    }
    return "Not Connected";
}

bool setup_ethernet() {
    // Hardware reset the W5500
    pinMode(ETH_RST, OUTPUT);
    digitalWrite(ETH_RST, LOW);
    delay(10);
    digitalWrite(ETH_RST, HIGH);
    delay(100);

    // Initialise the shared SPI bus with the correct MISO pin.
    // ETH_CLK/MOSI/MISO match the SD-card and OLED bus on BOARD_VER_V3.
    // Calling SPI.begin() here is safe even when the display already called it
    // with MISO=-1; this call adds MISO so Ethernet receive works correctly.
    SPI.begin(ETH_CLK, ETH_MISO, ETH_MOSI, -1);

    Ethernet.init(ETH_CS);
    Serial.println("ethernet: DHCP starting...");
    if (Ethernet.begin(eth_mac) == 0) {
        Serial.println("ethernet: DHCP failed");
        return false;
    }
    Serial.printf("ethernet: %s\n", Ethernet.localIP().toString().c_str());
    return true;
}

void loop_ethernet() {
    Ethernet.maintain();
}

#endif // ENABLE_ETHERNET
