#ifndef Axius6_h
#define Axius6_h

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <vector>

enum class DisplayType {
  oled128x64
};

class Axius {
    public:
        Axius() : display(Adafruit_SSD1306(128, 64, &Wire, -1)) {};
        void begin(DisplayType displayType, uint8_t scl, uint8_t sda);
        void tick(); //begin tick
        void endTick(); //end tick
        void setKeys(uint8_t Y_up, uint8_t Z_down, uint8_t X_ok);
        void updateKeys();
        void drawText(String text, uint8_t row);

        void setDisplayingParameters(String* params[], uint8_t count);
        void setDisplayingParameters(const std::vector<String> &params);

        static Axius* instance;
    private:
        DisplayType currentDisplayType;
        Adafruit_SSD1306 display;
        uint8_t scl, sda;
        uint8_t up_key_pin, down_key_pin, ok_key_pin;

        std::vector<String> curParameters;

        //constants
        const uint8_t MAX_PARAMETERS_FIT_IN_SCREEN = 6;
        const uint8_t ROW_HEIGHT = 8;
        const uint8_t COLUMN_TOP_SHIFT = 20;
};


#endif