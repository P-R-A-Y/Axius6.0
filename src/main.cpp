#include <Arduino.h>
#include <Axius.h>

Axius axius;

void setup() {
  axius.begin(DisplayType::oled128x64, D6, D5);

  std::vector<String> params = {"TEST 1", "SEX", "jopa"};

  axius.setDisplayingParameters(params);
}

void loop() {
  axius.tick();

  axius.endTick();
}