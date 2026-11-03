#include <Axius.h>

Axius* Axius::instance = nullptr;

//-------------------BASE-------------------

void Axius::begin(DisplayType displayType, uint8_t SCL, uint8_t SDA) {
    currentDisplayType = displayType;
    scl = SCL;
    sda = SDA;

    Wire.begin(sda, scl);
    Wire.setClock(400000);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    display.clearDisplay();
    
    instance = this;
}

void Axius::tick() {

    if (!curParameters.empty()) {
        display.setTextColor(WHITE);

        for (uint8_t i = 0; i < curParameters.size(); i++) {
            drawText(curParameters[i], i);
        }
    }
}

void Axius::endTick() {
    display.display();
    display.clearDisplay();
}







//-------------------DISPLAYING PARAMETERS-------------------

void Axius::setDisplayingParameters(String* params[], uint8_t count) {
    curParameters.clear();
    curParameters.shrink_to_fit();

    curParameters.reserve(count);
    for (uint8_t i = 0; i < count; i++) {
        curParameters.push_back(*params[i]);
    }
}

void Axius::setDisplayingParameters(const std::vector<String>& params) {
    curParameters.clear();
    curParameters.shrink_to_fit();

    curParameters.reserve(params.size());
    for (const String& s : params) {
        curParameters.push_back(s);
    }
}






//-------------------KEYS-------------------

void Axius::setKeys(uint8_t Y_up, uint8_t Z_down, uint8_t X_ok) {
    
}








//-------------------TEXT DISPLAY-------------------

void Axius::drawText(String text, uint8_t row) {
  uint8_t y = COLUMN_TOP_SHIFT + row * ROW_HEIGHT;
  display.setCursor(1,y);
  display.println(text);
}