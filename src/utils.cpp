#include "utils.h"
#include <LittleFS.h>
String composeClientID()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String reducedMac;

    for (int i = 3; i < 6; ++i)
    {
        reducedMac += String(mac[i], 16);
    }
    String clientId = "fabriarm-";
    clientId += reducedMac;
    return clientId;
}

bool formatLittleFS()
{
    log_w("need to format LittleFS: ");
    LittleFS.end();
    LittleFS.begin();
    log_i("Success: %d", LittleFS.format());
    return LittleFS.begin();
}
