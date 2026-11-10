#include <Axius.h>

extern "C" {
  #include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

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

    registerMode(&aboutMode);
    //tomenu();
}

void Axius::tick() {
    updateKeys();

    if (currentState == globalState::FIRSTTICK) {
        //drawText("booting up", 0);
        tomenu();
        requestDisplayUpdate();
        return;
    }
    if (currentState == globalState::MAINMENU) {
        //requestDisplayUpdate();
        if (isItemSelected()) {
            currentMode = getSelectedItemIndex();
            currentState = globalState::INMODE;
            closeList();
            modes[currentMode]->setup();
            requestDisplayUpdate();
        }
    }
    if (currentState == globalState::INMODE) {
        modes[currentMode]->tick();
    }

    if (isListVisible) {
        if (listItems.empty()) {
            //drawText("list is empty", 0);
            return;
        }
        if (isKeyPressed(KEYS::UP) && ItemCursor + 1 < listItems.size()) {
            requestDisplayUpdate();
            ItemCursor++;
            if (listItems.size() > MAX_ITEMS_FIT_IN_SCREEN) {
                if (ItemCursor + 1 - listScrollOffset > MAX_ITEMS_FIT_IN_SCREEN) listScrollOffset++;
            }
        }

        if (isKeyPressed(KEYS::DOWN) && ItemCursor > 0) {
            requestDisplayUpdate();
            ItemCursor--;
            if (listItems.size() > MAX_ITEMS_FIT_IN_SCREEN) {
                if (ItemCursor + 1 == listScrollOffset) listScrollOffset--;
            }
        }

        if (isKeyPressed(KEYS::OK)) {
            requestDisplayUpdate();
            isSelectionConfirmed = true;
            pickedItemIndex = ItemCursor;
        }
    }
}

void Axius::render() {
    if (!updateRequested) return;
    display.clearDisplay();

    if (currentState == globalState::FIRSTTICK) {
        drawText("booting up", 0);
    } else if (currentState == globalState::INMODE) {
        modes[currentMode]->render();
    }

    if (isListVisible) {
        if (listItems.empty()) {
            drawText("list is empty", ITEMS_ROW_MARGIN);
        } else {
            for (uint8_t i = 0; i < min(listItems.size() - listScrollOffset, (std::size_t)MAX_ITEMS_FIT_IN_SCREEN); i++) {
                drawTextSelector(listItems[i + listScrollOffset], i+ITEMS_ROW_MARGIN, i + listScrollOffset == ItemCursor );
            }
        }
    }

    updateRequested = false;
    frameReady = true;
}

void Axius::endTick() {
    if (frameReady) {
        display.display();
        frameReady = false;
    }
}



void Axius::registerMode(Mode* mode) {
    modes.push_back(mode);
}

void Axius::tomenu() {
    currentState = globalState::MAINMENU;

    if (isListVisible) closeList();

    listItems.reserve(modes.size());
    for (Mode* mode : modes) {
        addListItem(String(mode->getName()));
    }

    openList();

    up_key_state = 0;
    down_key_state = 0;
    ok_key_state = 0;

    stopSniffing();
}




//------------------WIFI METHODS-----------------------------

void Axius::setChannel(uint8_t channel) {
    wifi_set_channel(channel);
}

void Axius::sniffCallback(uint8_t* buf, uint16_t len) {
    if (instance) instance->onFrame(buf, len);
}

void Axius::startSniffing() {
    instance = this;
    wifi_fpm_do_wakeup();
    wifi_fpm_close();
    wifi_set_opmode(STATION_MODE);
    wifi_station_disconnect();
    wifi_promiscuous_enable(false);
    wifi_set_promiscuous_rx_cb(&Axius::sniffCallback);
    wifi_promiscuous_enable(true);
}

void Axius::stopSniffing() {
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(nullptr);

    queueHead = 0;
    queueTail = 0;
}

// НИГГЕР АЛАРМ!!! НЕЧИТАБЕЛЬНЫЙ КОД!!!
void Axius::onFrame(uint8_t *buf, uint16_t len) {
    if (len != sizeof(SnifferBuf2)) return; //если какойто левый чзх пакет пришел
    SnifferBuf2 *packet = reinterpret_cast<SnifferBuf2*>(buf);
    if (packet->count != 1) return; //Если вместо одного пришло несколько, хз вроди нужна проверка, а вроди ситуация не очень реалистичная
    uint16_t frameLen = packet->frameLen;  //размер фрейма по итогу
    if (frameLen < 36) return;
    uint16_t capturedLen = frameLen > 112 ? 112 : frameLen; //если размер неверный изза битого фрейма
    uint8_t *frame = packet->frame;



    
    uint16_t fc = frame[0] | (frame[1] << 8);
    uint8_t type = (fc >> 2) & 0x03;
    uint8_t subtype = (fc >> 4) & 0x0F;
    if (type != 0 || subtype != 8) return;// 0-Management 8-Beacon



    BeaconInfo beacon;

    memcpy(beacon.target_mac, frame + 4, 6); //целевой мак 
    memcpy(beacon.ap_mac, frame + 10, 6);    //тот кто высрал этот фрейм

    beacon.rssi = static_cast<int8_t>(packet->rxControl[0]);
    //beacon.channel = wifi_get_channel(); //фу так делать frame[36+2]

    uint16_t offset = 36;
    uint8_t parsecount = 0;

    while (offset + 2 <= capturedLen && parsecount < 4) {
        uint8_t id = frame[offset];
        uint8_t size = frame[offset + 1];

        offset += 2;

        if (offset + size > capturedLen) return;

        if (id == 0) {
            if (size > 32) return;
            memcpy(beacon.ssid, frame + offset, size);
            beacon.ssidLen = size;
            // if (size == 0) КТО ЭТО БЛЯ?
        } else if (id == 3) {
            beacon.channel = frame[offset];
        }

        offset += size;
        parsecount++;
    }

    uint8_t next = (queueHead + 1) % BEACON_BUFFER_SIZE; //я хз зачем этот код было так усложнять, этот бред собаки не я писал
    if (next == queueTail) return; //если не влезает - похуй, лишнее
    beaconQueue[queueHead] = beacon;
    queueHead = next;
}

bool Axius::beaconQueueIsEmpty() {
    return queueHead == queueTail;
}

bool Axius::popBeacon(BeaconInfo &result) {
    if (queueHead == queueTail) return false;
    result = beaconQueue[queueTail];
    queueTail = (queueTail + 1) % BEACON_BUFFER_SIZE; //толи фентаниловый код толи ЭЙАЙ слоп хз

    return true;
}



//-------------------DISPLAYING PARAMETERS-------------------

void Axius::openList() {
    isListVisible = true;
    requestDisplayUpdate();
    MAX_ITEMS_FIT_IN_SCREEN = DEAFULT_MAX_ITEMS_FIT_IN_SCREEN;
    ITEMS_ROW_MARGIN = 0;
}

void Axius::openList(uint8_t maxLines, uint8_t startRow) {
    isListVisible = true;
    requestDisplayUpdate();
    MAX_ITEMS_FIT_IN_SCREEN = maxLines;
    ITEMS_ROW_MARGIN = startRow;
}

void Axius::closeList() {
    listItems.clear();
    listItems.shrink_to_fit();

    ItemCursor = 0;
    listScrollOffset = 0;
    isSelectionConfirmed = false;
    pickedItemIndex = 0;
    MAX_ITEMS_FIT_IN_SCREEN = 5;
    ITEMS_ROW_MARGIN = 0;

    isListVisible = false;

    requestDisplayUpdate();
}

void Axius::setDisplayingLines(String* params[], uint8_t count) {
    listItems.reserve(count);
    for (uint8_t i = 0; i < count; i++) {
        listItems.push_back(*params[i]);
    }
    requestDisplayUpdate();
}

void Axius::addListItem(String param) {
    listItems.push_back(param);
    requestDisplayUpdate();
}

void Axius::setDisplayingLines(const std::vector<String>& params) {
    listItems.reserve(params.size());
    for (const String& s : params) {
        listItems.push_back(s);
    }
    requestDisplayUpdate();
}






//-------------------KEYS-------------------

void Axius::setKeys(uint8_t Y_up, uint8_t Z_down, uint8_t X_ok) {
    up_key_pin = Y_up;
    down_key_pin = Z_down;
    ok_key_pin = X_ok;

    pinMode(up_key_pin, INPUT_PULLUP);
    pinMode(down_key_pin, INPUT_PULLUP);
    pinMode(ok_key_pin, INPUT_PULLUP);
}

void Axius::updateKeys() { //вызывается каждый тик
    int voltageUP = digitalRead(up_key_pin);
    int voltageDOWN = digitalRead(down_key_pin);
    int voltageOK = digitalRead(ok_key_pin);

    //0 - не нажата
    //1 - нажата и удерживается
    //2 - отпущена, но состояние еще не считано

    if (voltageUP == LOW && up_key_state == 0) { //сейчас зажат, но в прошлом тике не был зажат
      up_key_state = 1;
    } else if (voltageUP == HIGH && up_key_state == 1) { //сейчас отпущен, но в прошлом тике был зажат
      up_key_state = 2;
    }

    if (voltageDOWN == LOW && down_key_state == 0) {
      down_key_state = 1;
    } else if (voltageDOWN == HIGH && down_key_state == 1) {
      down_key_state = 2;
    }

    if (voltageOK == LOW && ok_key_state == 0) {
      ok_key_state = 1;
    } else if (voltageOK == HIGH && ok_key_state == 1) {
      ok_key_state = 2;
    }
}

bool Axius::isKeyPressed(KEYS key) { //тоже вызывается каждый тик
    if ((key == KEYS::DOWN || key == KEYS::Z) && down_key_state == 2) {
        down_key_state = 0;
        return true;
    }
    if ((key == KEYS::UP || key == KEYS::Y) && up_key_state == 2) {
        up_key_state = 0;
        return true;
    }
    if ((key == KEYS::OK || key == KEYS::X) && ok_key_state == 2) {
        ok_key_state = 0;
        return true;
    }
    return false;
}

void Axius::resetKeys() {
    if (ok_key_state   == 2) ok_key_state = 0;
    if (up_key_state   == 2) up_key_state = 0;
    if (down_key_state == 2) down_key_state = 0;
}






//-------------------TEXT DISPLAY-------------------

void Axius::drawText(String text, uint8_t row) {
    display.setTextColor(WHITE);
    uint8_t y = COLUMN_TOP_SHIFT + row * ROW_HEIGHT;
    display.setCursor(0,y);
    display.println(text);
}

void Axius::drawTextSelector(String text, uint8_t row, bool isSelected) {
    display.setTextColor(WHITE);
    uint8_t y = COLUMN_TOP_SHIFT + row * ROW_HEIGHT;
    if (isSelected) {
        display.drawFastVLine(0, y, 7, WHITE);
        display.drawFastVLine(1, y, 7, WHITE);
    }
    display.setCursor(3,y);
    display.println(text);
}