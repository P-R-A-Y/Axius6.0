#ifndef Axius6_h
#define Axius6_h

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <vector>
#include "mode_base.h"

#define WIFI_CHANNEL_MIN 1
#define WIFI_CHANNEL_MAX 14

//prebuilt modes
#include "About_mode.h"

enum class DisplayType {
    oled128x64
};

enum class globalState {
    FIRSTTICK, MAINMENU, INMODE
};

enum class KEYS {
    OK, UP, DOWN, X, Y, Z
};

struct BeaconInfo {
    uint8_t ap_mac[6];
    uint8_t target_mac[6];
    uint8_t channel;
    int8_t  rssi;
    char    ssid[33];
    uint8_t ssidLen;
};

struct SnifferBuf2 {
    uint8_t rxControl[12];
    uint8_t frame[112];
    uint16_t count;
    uint16_t frameLen;
};

class Axius {
    public:
        Axius(const String deviceName) : display(Adafruit_SSD1306(128, 64, &Wire, -1)), aboutMode(this, 0xAA00) {
            this->deviceName = deviceName;
        };
        void begin(DisplayType displayType, uint8_t scl, uint8_t sda);
        void tick(); //begin tick
        void render();
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
        void requestDisplayUpdate() { updateRequested = true; };
        void openList();
        void openList(uint8_t maxLines, uint8_t startRow);
        void closeList();
        void setDisplayingLines(String* params[], uint8_t count);
        void addListItem(String param);
        void setDisplayingLines(const std::vector<String> &params);
        bool isItemSelected() { return isSelectionConfirmed; };
        uint8_t getSelectedItemIndex() { isSelectionConfirmed = false; return pickedItemIndex; };

        void startSniffing();
        void stopSniffing();
        void setChannel(uint8_t channel);
        bool beaconQueueIsEmpty();
        bool popBeacon(BeaconInfo &result);

        bool isSameMac(const uint8_t* cur, const uint8_t* tar) { return memcmp(cur, tar, 6) == 0; }

        static Axius* instance;

        const uint8_t DEAFULT_MAX_ITEMS_FIT_IN_SCREEN = 5;

        String deviceName;
    private:
        static void sniffCallback(uint8_t *buf, uint16_t len);
        void onFrame(uint8_t* buf, uint16_t len);
        const static uint8_t BEACON_BUFFER_SIZE = 32;
        BeaconInfo beaconQueue[BEACON_BUFFER_SIZE];
        volatile uint8_t queueHead = 0;
        volatile uint8_t queueTail = 0;


        bool updateRequested = true;
        bool frameReady = false;

        DisplayType currentDisplayType;
        Adafruit_SSD1306 display;
        uint8_t scl, sda;
        uint8_t up_key_pin, down_key_pin, ok_key_pin;
        uint8_t up_key_state = 0, down_key_state = 0, ok_key_state = 0; // 0 - не нажат/отжат.   1 - нажат/удерживается.  2 - отпущен, готов к считыванию

        globalState currentState = globalState::FIRSTTICK;
        std::vector<Mode*> modes;
        uint8_t currentMode = 0;
        

        //lines display
        std::vector<String> listItems;
        uint8_t listScrollOffset = 0;
        uint8_t ItemCursor = 0;
        bool isSelectionConfirmed = false;
        uint8_t pickedItemIndex = 0;
        bool isListVisible = false;
        uint8_t MAX_ITEMS_FIT_IN_SCREEN = 5;
        uint8_t ITEMS_ROW_MARGIN = 0;

        //constants
        const uint8_t ROW_HEIGHT = 8;
        const uint8_t COLUMN_TOP_SHIFT = 20;

        //prebuilt modes
        About aboutMode;
};



#endif