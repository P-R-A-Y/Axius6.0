#include <Arduino.h>
#include <Axius.h>

const int ledpin = D8, laserpin = D4;

Axius axius("Shimmer v6");

#include "wifi_ap_scanner.h"
WIFIScanner scanner(&axius, 0x0000);

void setup() {
  Serial.begin(115200);
  
  axius.setKeys(D2, D1, D7);
  axius.begin(DisplayType::oled128x64, D6, D5);

  axius.registerMode(&scanner);

  pinMode(ledpin, OUTPUT);
  pinMode(laserpin, OUTPUT);
  analogWrite(ledpin, LOW);
  digitalWrite(laserpin, LOW);
}

void loop() {
  axius.tick();

  axius.render();

  axius.endTick();
}