#ifndef beacon_h
#define beacon_h

#include <mode_base.h>
#include <Tools.h>

extern const uint8_t ssidCount;
extern const char* ssidPack[];

class WIFIBeacon : public Mode {
public:
    WIFIBeacon(Axius* axiusInstance, uint16_t ID) : Mode(axiusInstance, ID) {};

    void tick() override;
    void render() override;
    void setup() override;
    const char* getName() const override { return "SingleName WIFI Spam"; };
    const char* const* getRequiredParameters() override { return nullptr; };
    uint8_t getRequiredParametersCount() override { return 0; };

    void attackTick();
private:
    bool cooldown1sec = false;
    bool applyWPA2 = false;
    uint8_t beaconsCount = 40;
    uint8_t currentSSID = 0;

    //dynamic data
    BeaconPacketConstructor* packets = nullptr;
    uint32_t lastAttackSend = 0;
    uint32_t lastMacUpdate = 0;
    uint8_t macSpecialByte = 0x00;
    uint8_t mac[6] = {0xFA, 0xBA, 0xCA, 0xBA, 0x00, 0x00};
    bool exitscreen = false;
    uint32_t exitscreenConfirmTime = 0;



    enum class State {
        PICK_NAMEPACK, ATTACK
    };
    State state = State::PICK_NAMEPACK;

    void stopAttack() {
        state = State::PICK_NAMEPACK;
        Serial.println(-1);
        if (packets != nullptr) {
            Serial.println(-2);
            for (uint8_t i = 0; i < beaconsCount; i++) {
                packets[i].manualDestructPacket();
            }
            Serial.println(-3);
            delete[] packets;
            packets = nullptr;
            Serial.println(-4);
        }
    }

    void beginAttack(bool coldown1sec_ = false, bool applyWPA2_ = false, uint8_t beaconsCount_ = 40, int8_t currentSSID_ = -1) {
        cooldown1sec = coldown1sec_;
        applyWPA2 = applyWPA2_;
        beaconsCount = beaconsCount_;
        if (currentSSID_ != -1) currentSSID = currentSSID_;
        state = State::ATTACK;
        Serial.println(0);
        packets = new BeaconPacketConstructor[beaconsCount];
        uint8_t channel = 0;
        for (uint8_t i = 0; i < beaconsCount; i++) {
            if (++channel > 14 || i == 0) channel = 1;
            mac[5] = i;
            packets[i].setProperties(Tools::readPROGMEMString(ssidPack[currentSSID]), applyWPA2, i, mac, cooldown1sec, false);
            packets[i].setChannel(channel);
            //packets[i].setMac(mac);
        }
        Serial.println(5);
        lastMacUpdate = millis();
    }
};

#endif