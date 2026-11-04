#pragma once
#include <Arduino.h>

class Axius;

class Mode {
public:
    Mode(Axius* instance, uint16_t ID) : axius(instance), ID(ID) {}

    virtual void tick() = 0;
    virtual void setup() = 0;
    virtual const char* getName() const = 0;

    uint16_t getID() const { return ID; }

    Axius* axius = nullptr;

private:
    uint16_t ID;
};