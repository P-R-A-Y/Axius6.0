#pragma once
#include <Arduino.h>

class Tools {
public:
    static bool isMacValid(const uint8_t* arr) {
        return memcmp(arr, BAD_MAC, 6) != 0;
    }
    static bool isSameMac(const uint8_t* cur, const uint8_t* tar) {
        return memcmp(cur, tar, 6) == 0;
    }

    static String strmac(uint8_t* mac) {
        String result = "";
        for (int i = 0; i < 6; i++) {
            if (mac[i] < 16) result += "0";
            result += String(mac[i], HEX);
            if (i < 5) result += ":";
        }
        return result;
    }

    static String charArrayToString(char *array, size_t arraySize) {
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

    static String uint8ArrayToString(uint8_t *array, size_t arraySize) {
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

    static String readPROGMEMString(const char* str) {
        String result;
        char c;
        while ((c = pgm_read_byte(str++)) != 0) {
        result += c;
        }
        return result;
    }

private:
    Tools() = delete;

    inline static const uint8_t BAD_MAC[6] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
};


const char substrings[] PROGMEM = {
  '0','1','2','3','4','5','6','7','8','9',
  'a','b','c','d','e','f','g','h','i','j',
  'k','l','m','n','o','p','q','r','s','t',
  'u','v','w','x','y','z','z','y','x','w',
  'v','u','t','s','r','q','p','o','n','m',
  'l','k','j','i','h','g','f','e','d','c',
  'b','a','9','8','7','6','5','4','3','2',
  '1','0','q','w','e','r','t','y','u','i',
  'o','p','a','s','d','f','g','h','j','k',
  'l','z','x','c','v','b','n','m','1','3',
  '2','4','3','5','4','6','5','7','6','8',
  '0'};

const uint8_t beaconPacketTemplate[] PROGMEM = {
    /*  0 - 3  */ 0x80, 0x00, 0x00, 0x00,             // Type/Subtype/duration/duration: managment beacon frame
    /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
    /* 10 - 15 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
    /* 16 - 21 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
    // Fixed parameters
    /* 22 - 23 */ 0x00, 0x00,                         // Fragment & sequence number (will be done by the SDK)
    /* 24 - 31 */ 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
    /* 32 - 33 */ 0xe8, 0x03,                         // Interval: 0x64, 0x00 => every 100ms - 0xe8, 0x03 => every 1s
    /* 34 - 35 */ 0x31, 0x00,                         // capabilities Information
    // Tagged parameters
    // SSID parameters
    /* 36 - 37 */ 0x00, 0x20,                         // Tag: Set SSID length, Tag length: 32
    /* 38 - 69 */
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,                           // SSID
    // Supported Rates
    /* 70 - 71 */ 0x01, 0x08,                         // Tag: Supported Rates, Tag length: 8 
    /* 72 */ 0x82,                    // 1(B)
    /* 73 */ 0x84,                    // 2(B)
    /* 74 */ 0x8b,                    // 5.5(B)
    /* 75 */ 0x96,                    // 11(B)
    /* 76 */ 0x24,                    // 18
    /* 77 */ 0x30,                    // 24
    /* 78 */ 0x48,                    // 36
    /* 79 */ 0x6c,                    // 54
    // Current Channel
    /* 80 - 81 */ 0x03, 0x01,         // Channel set, length
    /* 82 */      0x01,               // Current Channel 
    // RSN information
    /*  83 -  84 */ 0x30, 0x18,
    /*  85 -  86 */ 0x01, 0x00,
    /*  87 -  90 */ 0x00, 0x0f, 0xac, 0x02,
    /*  91 -  92 */ 0x02, 0x00,
    /*  93 - 100 */ 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04, /*Fix: changed 0x02(TKIP) to 0x04(CCMP) is default. WPA2 with TKIP not supported by many devices*/
    /* 101 - 102 */ 0x01, 0x00,
    /* 103 - 106 */ 0x00, 0x0f, 0xac, 0x02,
    /* 107 - 108 */ 0x00, 0x00
};


class BeaconPacketConstructor {
  public:
    void setProperties(String name, bool wpa2, int8_t hashTagIndex, uint8_t* mac, bool cd1s, bool hashTagIndexBackwards) {
        name.replace("{i}", String(hashTagIndex));
        if (name.indexOf("{nohash}") >= 0) {
            name.replace("{nohash}", "");
            hashTagIndex = -1;
        }
        if (hashTagIndex == -1) nameSize = min(name.length(), (uint) 32);
        else                    nameSize = min(name.length() + 4, (uint) 32);
        packetSize = 109 - 32 + nameSize;
        if (!wpa2) packetSize -= 26;

        if (packet) {
            delete[] packet;
            packet = nullptr;
        }
        packet = new uint8_t[packetSize];

        memcpy_P(&packet[0], &beaconPacketTemplate[0], 38);                            // front part
        memcpy(&packet[10], mac, 6);                                                   // set mac
        memcpy(&packet[16], mac, 6);                                                   // set mac
        packet[37] = nameSize;                                                         // set name size
        if (cd1s) {
            packet[32] = 0xe8; 
            packet[33] = 0x03; 
        } else {
            packet[32] = 0x64; 
            packet[33] = 0x00; 
        }
        if (wpa2) packet[34] = 0x31;                                                   // set wpa enabled
        else      packet[34] = 0x21;                                                   // set wpa disabled
        memcpy(&packet[((hashTagIndex > -1 && hashTagIndexBackwards) ? 42 : 38)], &name[0], nameSize - (hashTagIndex == -1 ? 0 : 4));        // set name
        if (hashTagIndex > -1) {
            if (hashTagIndexBackwards) {
            packet[38 + 0] = pgm_read_byte(&substrings[hashTagIndex]);
            packet[38 + 1] = pgm_read_byte(&substrings[hashTagIndex + 1]);
            packet[38 + 2] = '#';
            packet[38 + 3] = ' ';
            } else {
            packet[38 + nameSize - 4] = ' ';
            packet[38 + nameSize - 3] = '#';
            packet[38 + nameSize - 2] = pgm_read_byte(&substrings[hashTagIndex]);
            packet[38 + nameSize - 1] = pgm_read_byte(&substrings[hashTagIndex + 1]);
            }
        }
        memcpy_P(&packet[38 + nameSize], &beaconPacketTemplate[70], wpa2 ? 39 : 13);   // set back part
        if (wpa2) packet[34] = 0x31;                                                   // set wpa2
        else packet[34] = 0x21;                                                        // set no wpa2
    };

    void setChannel(uint8_t c) {
        channel = c;
        packet[82 - 32 + nameSize] = channel;
    };

    void setMac(uint8_t* mac) {
        memcpy(&packet[10], mac, 6);
        memcpy(&packet[16], mac, 6);
    }

    uint8_t* getBytes() { return packet; };

    uint8_t getPacketSize() { return packetSize; };

    uint8_t getChannel() { return channel; }

    void manualDestructPacket() {
      if (packet) {
        delete[] packet;
        packet = nullptr;
      }
    }
  private:
    uint8_t* packet = nullptr;
    uint8_t nameSize, packetSize, channel;
};