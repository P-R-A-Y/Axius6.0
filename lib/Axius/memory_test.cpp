#include "memory_test.h"
#include "Axius.h"

void memorytest::setup() {
    
};

void memorytest::tick() {
    if (axius->isKeyPressed(KEYS::OK)) {
        memory.setup();
        axius->requestDisplayUpdate();
        Serial.println("test");
    }
    if (axius->isKeyPressed(KEYS::UP)) {
        uint8_t ex = 21;
        memory.setParameter("test.value", &ex, 1);
        Serial.println("set TP as 21");
        memory.getParameter("test.value", &testParameterValue, 1);
        Serial.println("get TP value: "+String(testParameterValue));
    }
    if (axius->isKeyPressed(KEYS::DOWN)) {
        Serial.println(memory.getParameter("test.value", &testParameterValue, 1));
        Serial.println("got TP value: "+String(testParameterValue));
    }
};

void memorytest::render() {
    axius->drawText("status: "+String(memory.status)+" TP: "+String(testParameterValue), 0);
    axius->drawText("ok-setup up-TP=21 dwn-readTP", 1);
};