#pragma once
#include "mode_base.h"
#include <vector>

class WIFIScanner : public Mode {
public:
    WIFIScanner(Axius* axiusInstance, uint16_t ID) : Mode(axiusInstance, ID) {};

    void tick() override;
    void render() override;
    void setup() override;
    const char* getName() const override;
    const char* const* getRequiredParameters() override { return nullptr; };
    uint8_t getRequiredParametersCount() override { return 0; };
private:
    void exit() {
        state = State::BINDING;
        accessPoints.clear();
        accessPoints.shrink_to_fit();
    }

    enum class State {
        BINDING, LISTENING
    };

    struct AccessPoint {
        uint8_t mac[6];
        uint8_t channel;
        int8_t  lastrssi;
        char    ssid[33];
        uint8_t ssidLen;
    };

    State state = State::BINDING;

    std::vector<AccessPoint> accessPoints;

    uint16_t lastChannelJump = 0;
    uint8_t currentChannel = 1;
};