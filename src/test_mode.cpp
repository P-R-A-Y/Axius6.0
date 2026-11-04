#include "test_mode.h"
#include "Axius.h"

void About::setup() {}

const char* About::getName() const {
    return "About";
}

void About::tick() {
    axius->drawText("Powered by Axius 6.0", 0);
    axius->drawText("press OK to exit", 1);

    if (axius->isKeyPressed(KEYS::OK)) axius->tomenu();
    
    axius->resetKeys();
}