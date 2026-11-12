#pragma once
#include "mode_base.h"
#include "Memory.h"

class memorytest : public Mode {
    public:
        memorytest(Axius* axiusInstance) : Mode(axiusInstance, 0xFFFA), memory() {};

        void tick() override;
        void render() override;
        void setup() override;
        const char* getName() const override { return "memorytest"; };
        const char* const* getRequiredParameters() override { return nullptr; };
        uint8_t getRequiredParametersCount() override { return 0; };
    private:
        Memory memory;
        uint8_t testParameterValue = 42;
};