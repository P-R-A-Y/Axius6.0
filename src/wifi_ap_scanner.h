#pragma once
#include "mode_base.h"
#include <vector>

class WIFIScanner : public Mode {
public:
    WIFIScanner(Axius* axiusInstance, uint16_t ID) : Mode(axiusInstance, ID) {}

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

    const uint8_t BAD_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    bool isMacGood(const uint8_t* arr) {
        return memcmp(arr, BAD_MAC, 6) != 0;
    };

    String strmac(uint8_t* mac) {
        String result = "";
        for (int i = 0; i < 6; i++) {
            if (mac[i] < 16) result += "0";
            result += String(mac[i], HEX);
            if (i < 5) result += ":";
        }
        return result;
    }

    String charArrayToString(char *array, size_t arraySize) {
        String result = "";
        for (size_t i = 0; i < arraySize; i++) {
            if (array[i] != 0) {
                result += array[i];
            } else {
                break;
            }
        }
        return result;
    }

    String uint8ArrayToString(uint8_t *array, size_t arraySize) {
        String result = "";
        for (size_t i = 0; i < arraySize; i++) {
            if (array[i] != 0) {
                result += static_cast<char>(array[i]);
            } else {
                break;
            }
        }
        return result;
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