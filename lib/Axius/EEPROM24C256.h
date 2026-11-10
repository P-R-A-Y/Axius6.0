
#pragma once

#include <Arduino.h>
#include <Wire.h>

class EEPROM24C256 {
public:
    static constexpr uint8_t DEVICE_ADDRESS = 0x50;

    static constexpr uint16_t TOTAL_SIZE = 32768;
    static constexpr uint8_t PAGE_SIZE = 64;

    static constexpr uint8_t MAX_WRITE_SIZE = 30;
    static constexpr uint8_t MAX_READ_SIZE = 32;

    explicit EEPROM24C256(uint8_t deviceAddress = DEVICE_ADDRESS) : deviceAddress(deviceAddress) {}

    bool readByte(uint16_t address, uint8_t& value) {
        return readBytes(address, &value, 1);
    }

    bool readBytes(uint16_t address, uint8_t* buffer, size_t length) {
        if (!isValidRange(address, length)) return false;
        if (length == 0) return true;
        if (buffer == nullptr) return false;

        size_t offset = 0;

        while (offset < length) {
            size_t chunk = min(length - offset, (size_t)MAX_READ_SIZE);

            uint16_t currentAddress = address + offset;

            Wire.beginTransmission(deviceAddress);
            Wire.write((uint8_t)(currentAddress >> 8));
            Wire.write((uint8_t)(currentAddress & 0xFF));

            // false: не отправляем STOP, нужен repeated START.
            if (Wire.endTransmission(false) != 0) {
                return false;
            }

            uint8_t received = Wire.requestFrom(deviceAddress, (uint8_t)chunk);

            if (received != chunk) {
                while (Wire.available()) {
                    Wire.read();
                }
                return false;
            }

            for (size_t i = 0; i < chunk; i++) {
                if (!Wire.available()) {
                    return false;
                }

                buffer[offset + i] = Wire.read();
            }

            offset += chunk;
        }

        return true;
    }

    bool writeByte(uint16_t address, uint8_t value) {
        return writeBytes(address, &value, 1);
    }

    bool writeBytes(uint16_t address, const uint8_t* data, size_t length) {
        if (!isValidRange(address, length)) return false;
        if (length == 0) return true;
        if (data == nullptr) return false;

        size_t offset = 0;

        while (offset < length) {
            uint16_t currentAddress = address + offset;

            size_t pageRemaining = PAGE_SIZE - (currentAddress % PAGE_SIZE);

            size_t chunk = min(length - offset, min(pageRemaining, (size_t)MAX_WRITE_SIZE));

            Wire.beginTransmission(deviceAddress);
            Wire.write((uint8_t)(currentAddress >> 8));
            Wire.write((uint8_t)(currentAddress & 0xFF));

            size_t written = Wire.write(data + offset, chunk);

            if (written != chunk) {
                Wire.endTransmission();
                return false;
            }

            if (Wire.endTransmission() != 0) {
                return false;
            }

            if (!waitReady()) {
                return false;
            }

            offset += chunk;
        }

        return true;
    }

private:
    uint8_t deviceAddress;

    static constexpr uint8_t WRITE_TIMEOUT = 10;

    bool isValidRange(uint16_t address, size_t length) const {
        return (uint32_t)address + length <= TOTAL_SIZE;
    }

    bool waitReady() {
        uint32_t startTime = millis();

        while (millis() - startTime < WRITE_TIMEOUT) {
            Wire.beginTransmission(deviceAddress);
            uint8_t result = Wire.endTransmission();
            if (result == 0) {
                return true;
            }

            // 2 = адрес устройства не подтвержден
            if (result != 2) {
                return false;
            }

            yield();
        }

        return false;
    }
};
