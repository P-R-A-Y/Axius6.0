#include "wifi_beacon.h"
#include <Axius.h>
#include <Tools.h>

const uint8_t ssidCount PROGMEM = 9;
const char* ssidPack[] PROGMEM = {
  "www.pray-xdd.ru",
  "DESTROYTHISWORLD",
  "R A Z' E B A T O R",

  "И3И ИHET {i}",
  "TOILET SPY CAM {i}",
  "Y MEHЯ {i} IQ",

  "𓀿 <- saddam hussein", 
  "42 БРАТУХА",
  "FUCKYOUWIFI"
};

void WIFIBeacon::setup() {
    stopAttack();
    axius->beginFreedomWIFI();
    axius->openList();
    for (uint8_t i = 0; i < ssidCount; i++) {
        axius->addListItem(Tools::readPROGMEMString(ssidPack[i]));
    }
}

void WIFIBeacon::render() {
    if (state == State::ATTACK) {
        axius->drawTextC("> ATTACK <", 0);
        
        if (exitscreen) axius->drawTextC("exit? [OK] - confirm", 4);
        else            axius->drawTextC("[OK] - exit", 4);
    }
}

void WIFIBeacon::tick() {
    if (state == State::PICK_NAMEPACK) {
        if (axius->isItemSelected()) {
            currentSSID = axius->getSelectedItemIndex();
            axius->closeList();
            axius->setAutoScreenUpdate(1000);
            beginAttack();
            axius->requestDisplayUpdate();
            axius->resetKeys();
        }
    } else if (state == State::ATTACK) {
        attackTick();

        if (millis() - exitscreenConfirmTime > 1500)  {
            exitscreen = false;
            axius->requestDisplayUpdate();
        }

        if (axius->isKeyPressed(KEYS::OK)) {
            axius->requestDisplayUpdate();
            if (exitscreen) {
                stopAttack();
                axius->tomenu();
            } else {
                exitscreenConfirmTime = millis();
                exitscreen = true;
            }
        }

        axius->resetKeys();
    }
}

void WIFIBeacon::attackTick() {
    if (millis() - lastMacUpdate > 15000) {
        lastMacUpdate = millis();

        macSpecialByte++;
        for (uint8_t i = 0; i < beaconsCount; i++) {
            mac[4] = macSpecialByte;
            mac[5] = i;
            packets[i].setMac(mac);
        }
    }

    if (millis() - lastAttackSend > (cooldown1sec ? 1000 : 100)) {
        lastAttackSend = millis();

        for (uint8_t i = 0; i < beaconsCount; i++) {
            axius->setChannel(packets[i].getChannel());
            
            for (int k = 0; k < 3; k++) {
                bool sent = axius->sendWifiFrame(packets[i].getBytes(), packets[i].getPacketSize());

                if (applyWPA2) delay(2);
                else delay(1);

                if (sent) break;
            }
        }
    }
}