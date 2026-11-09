#include "About_mode.h"
#include "Axius.h"

void About::setup() {}

const char* About::getName() const {
    return "About";
}

void About::render() {
    axius->requestDisplayUpdate();
    axius->drawText(axius->deviceName, 0);
    axius->drawText("Powered by Axius 6.0", 1);
    
    axius->drawText("press OK to exit", 4);
}

void About::tick() {
    if (axius->isKeyPressed(KEYS::OK)) axius->tomenu();
    
    axius->resetKeys();
}