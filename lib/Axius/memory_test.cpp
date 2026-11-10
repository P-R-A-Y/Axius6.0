#include "memory_test.h"
#include "Axius.h"

void memorytest::setup() {
    
};

void memorytest::tick() {
    if (axius->isKeyPressed(KEYS::OK)) {
        memory.setup();
        axius->requestDisplayUpdate();
    }
    if (axius->isKeyPressed(KEYS::UP)) axius->tomenu();
};

void memorytest::render() {
    axius->drawText("ok - setup. status: "+String(memory.status), 0);
};