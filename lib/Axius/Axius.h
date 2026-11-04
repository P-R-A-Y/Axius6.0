#ifndef Axius6_h
#define Axius6_h

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <vector>
#include "mode_base.h"

enum class DisplayType {
    oled128x64
};

enum class globalState {
    FIRSTTICK, MAINMENU, INMODE
};

enum class KEYS {
    OK, UP, DOWN, X, Y, Z
};

class Axius {
    public:
        Axius() : display(Adafruit_SSD1306(128, 64, &Wire, -1)) {};
        void begin(DisplayType displayType, uint8_t scl, uint8_t sda);
        void tick(); //begin tick
        void endTick(); //end tick
        void setKeys(uint8_t Y_up, uint8_t Z_down, uint8_t X_ok);
        void updateKeys();
        bool isKeyPressed(KEYS key);
        void resetKeys(); //На случай если в данный момент нажатия не считываются. Чтобы при переходе на другой экран они не засчитывались с нулевой

        void tomenu();
        void registerMode(Mode* mode);

        //textDraw
        void drawText(String text, uint8_t row);
        void drawTextSelector(String text, uint8_t row, bool isselected);

        //lines display
        void openLinesDisplaying();
        void closeLinesDisplaying();
        void setDisplayingLines(String* params[], uint8_t count);
        void setDisplayingLines(const std::vector<String> &params);
        bool isLinePicked() { return linePicked; };
        uint8_t whichLineIsPicked() { linePicked = false; return currentPickedLine; };

        static Axius* instance;
    private:
        DisplayType currentDisplayType;
        Adafruit_SSD1306 display;
        uint8_t scl, sda;
        uint8_t up_key_pin, down_key_pin, ok_key_pin;
        uint8_t up_key_state = 0, down_key_state = 0, ok_key_state = 0; // 0 - не нажат/отжат.   1 - нажат/удерживается.  2 - отпущен, готов к считыванию

        globalState currentState = globalState::FIRSTTICK;
        std::vector<Mode*> modes;
        uint8_t currentMode = 0;
        

        //lines display
        std::vector<String> curParameters;
        uint8_t currentLinesMargin = 0;
        uint8_t currentLinesCursor = 0;
        bool linePicked = false;
        uint8_t currentPickedLine = 0;
        bool linesDisplaying = false;

        //constants
        const uint8_t MAX_LINES_FIT_IN_SCREEN = 5;
        const uint8_t ROW_HEIGHT = 8;
        const uint8_t COLUMN_TOP_SHIFT = 20;
};


#endif