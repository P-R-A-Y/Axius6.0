#include "wifi_ap_scanner.h"
#include "Axius.h"

void WIFIScanner::setup() {}

const char* WIFIScanner::getName() const {
    return "WIFI AP Scanner";
}

void WIFIScanner::render() {
    if (state == State::LISTENING) {
        axius->drawText("chanel:"+String(currentChannel)+" APs:"+String(accessPoints.size()), 0);
    }
}

void WIFIScanner::tick() {
    if (state == State::BINDING) {
        axius->requestDisplayUpdate();

        axius->openList(axius->DEAFULT_MAX_ITEMS_FIT_IN_SCREEN-1, 1);

        axius->startSniffing();
        state = State::LISTENING;
    } else if (state == State::LISTENING) {
        if (millis() - lastChannelJump > 310) {
            lastChannelJump = millis();
            if (++currentChannel >= WIFI_CHANNEL_MAX) currentChannel = WIFI_CHANNEL_MIN;
            axius->setChannel(currentChannel);
            axius->requestDisplayUpdate();
        }

        if (axius->isKeyPressed(KEYS::OK)) {
            exit();
            axius->tomenu();
        }

        if (!axius->beaconQueueIsEmpty()) {
            BeaconInfo beacon;
            while (axius->popBeacon(beacon)) {
                bool found = false;
                for (uint16_t i = 0; i < accessPoints.size(); i++) {
                    if (axius->isSameMac(accessPoints[i].mac, beacon.ap_mac)) {
                        accessPoints[i].lastrssi = beacon.rssi;
                        found = true;
                        break;
                    }
                }
                if (found) continue;

                AccessPoint beaconframe;

                memcpy(beaconframe.mac, beacon.ap_mac, 6);
                beaconframe.channel = beacon.channel;
                beaconframe.lastrssi = beacon.rssi;
                memcpy(beaconframe.ssid, beacon.ssid, beacon.ssidLen);
                beaconframe.ssidLen = beacon.ssidLen;

                accessPoints.push_back(beaconframe);

                axius->addListItem(beaconframe.ssidLen == 0 ? strmac(beaconframe.mac) : charArrayToString(beaconframe.ssid, beaconframe.ssidLen));
            }
        }
    }
}