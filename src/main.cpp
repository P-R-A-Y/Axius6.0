#include <Arduino.h>
#include <Axius.h>

Axius axius;

#include "test_mode.h"
static About aboutMode(&axius, 0x0000);

void setup() {
  axius.setKeys(D2, D1, D7);
  axius.begin(DisplayType::oled128x64, D6, D5);
  axius.registerMode(&aboutMode);

  /*std::vector<String> params = {"TEST 1", "SEX", "jopa", "TEST 2", "SEX2", "jopa2", "TEST 3", "SEX3", "jopa3"};

  axius.openLinesDisplaying();
  axius.setDisplayingLines(params);*/
}

void loop() {
  axius.tick();

  axius.endTick();
}