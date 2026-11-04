#pragma once
#include "mode_base.h"

class About : public Mode {
public:
    About(Axius* axiusInstance, uint16_t ID) : Mode(axiusInstance, ID) {}

    void tick() override;
    void setup() override;
    const char* getName() const override;
};