#include <Arduino.h>
#include "EEPROM24C256.h"

#ifndef Memory_h
#define Memory_h

static const char* errors = {"READ_RECORD_HEADER"};
enum class IOERROR {
    READ_RECORD_HEADER
};

enum class MemoryType : uint8_t {
    BOOL = 0,
    UINT8,
    INT8,
    UINT16,
    INT16,
    UINT32,
    INT32,
    FLOAT,
    STRING,
    UINT8_ARRAY
};

enum MemoryFlags : uint8_t {
    LOAD_ON_STARTUP = 1 << 0
};

struct ParameterDefinition {  // скелеты параметров объявленных в прошивке
    const char* name;
    MemoryType type;

    const void* defaultValue;
    uint16_t valueSize;
    uint16_t valueAddress;
    uint16_t defaultValueAddress;
    bool isCoreParameter;

    uint8_t flags;
};

struct MemoryIndex { // скелеты параметров (общие)
    uint16_t address;
    uint16_t recordSize;

    uint16_t valueAddress;
    uint16_t defaultValueAddress;
    uint16_t valueSize;

    uint8_t flags;
    uint8_t nameSize;
    MemoryType type;

    bool isOnEEPROM;
    bool isCoreParameter;

    char name[19];
};

struct MemoryParameter { //параметры (общие)
    void* value = nullptr;
    void* defaultValue = nullptr; //нужен ли?

    bool loaded = false;
};


//default parameters----------------------------------------
static const uint8_t defaultBrightness = 100;

static ParameterDefinition coreParametersBuiltin[] = {
    {
        "display.brightness",
        MemoryType::UINT8,
        &defaultBrightness,
        sizeof(defaultBrightness),
        0,
        0,
        true,
        LOAD_ON_STARTUP
    }
};
static ParameterDefinition wrapperParametersBuiltin[] = {}; //TODO придумать когда его заполнять

static const uint8_t coreParameterCount = sizeof(coreParametersBuiltin) / sizeof(coreParametersBuiltin[0]);
static const uint8_t wrapperParameterCount = 0;

static uint8_t coreParameterCount_onEEPROM = 0;
static uint8_t wrapperParameterCount_onEEPROM = 0;

static uint8_t SIGNATURE[] = {
    0x42, 0x42, 0x42, 0x42, 0x42,      // (подпись) 42 братуха
    'a', 'x', 'i', 'u', 's',           // (подпись) name 5b max
    0x00,                              // coreParameterCount on EEPROM
    0x00,                              // wrapperParameterCount on EEPROM
    0x00, 0x00, 0x00, 0x00, 0x00,      // 
    0xFA, 0xBA, 0xCA, 0xBA, 0x08, 0x01 // password
};

class Memory {
public:
    static constexpr uint16_t TOTAL_SIZE = 32768;                         // размер диска
    static constexpr uint16_t METADATA_START = 0;                         // адрес хеадера диска
    static constexpr uint16_t METADATA_SIZE = 128;                        // размер хеадера
    static constexpr uint16_t CORE_START = 128;                           // адрес начала памяти для параметров axius
    static constexpr uint16_t CORE_SIZE = 2048;                           // их размер
    uint16_t CORE_END = CORE_START;                                       //
    static constexpr uint16_t WRAPPER_START = 2176;                       // адрес начала памяти для параметров режимов
    static constexpr uint16_t WRAPPER_SIZE = TOTAL_SIZE - WRAPPER_START;  // их размер
    uint16_t WRAPPER_END = WRAPPER_START;                                 //
    static constexpr uint8_t  FLAG_MARKER_MASK = 0b00000011;              // маска-маркер параметра
    static constexpr uint8_t  FLAG_MARKER = 0b00000001;                   // маркер во флаге о том что запись параметра валидна
    static constexpr uint8_t  FLAG_DEAD = 0b00000100;                     // флаг мертвого параметра
    static constexpr uint8_t  FLAG_TYPE_MASK = 0b01111000;                // флаг типа параметра 4 бита 9/16 разных видов максимум
    static constexpr uint8_t  FLAG_LOAD_ON_STARTUP = 0b10000000;          // флаг обязательного для прочитки параетра
    static constexpr uint8_t  MAX_PARAMETER_COUNT = 100;

    EEPROM24C256 eeprom;
    uint16_t status = 0;

    Memory() : eeprom() {};

    void setup() {
        status = 1;
        if (isBlank()) {
            if (!formatBlankDisk()) {
                status = 2;
                return;
            }
        }

        if (!readMetadata()) {
            status = 3;
            return;
        }

        mapParametersFromEEPROM();
        status = 4;
    }

    bool isBlank() {
        uint8_t buffer[sizeof(SIGNATURE)];
        if (!eeprom.readBytes(METADATA_START, buffer, sizeof(buffer))) {
            return false; //read error
        }
        if (memcmp(buffer, SIGNATURE, 10) != 0) return true; //сравниваем сигнатуру
        if (memcmp(&buffer[17], &SIGNATURE[17], sizeof(SIGNATURE)-17) != 0) return true; //сравниваем пароль

        return false;
    }

    /*MemoryParameter* getParameter(const char* name) {
        if (name == nullptr) return nullptr;

        // Ищем с конца, чтобы последняя живая запись имела приоритет над предыдущими.
        for (int32_t i = parameterCount - 1; i >= 0; i--) {
            MemoryIndex& info = memoryMap[i];

            if (isDead(info.flags)) continue;
            if (strcmp(info.name, name) != 0) continue;
            if (!loadParameter(i)) return nullptr;

            return &parameters[i];
        }

        return nullptr;
    }*/

    bool getParameter(const char* name, void* output, uint16_t outputSize) {
        int index = -1;
        for (uint8_t i = 0; i < parameterCount; i++) {
            if (memcmp(memoryMap[i].name, name, min(strlen(name), (size_t)18)) == 0) index = i;
        }
        if (index < 0) return false;

        MemoryIndex& info = memoryMap[index];
        if (outputSize < info.valueSize) return false;

        memcpy(output, parameters[index].value, info.valueSize);
        return true;
    }

private:
    MemoryIndex* memoryMap = nullptr;       //разметка параметров
    MemoryParameter* parameters = nullptr;  //сами параметры
    uint8_t parameterCount = 0; //общее количество

    bool readMetadata() {
        eeprom.readByte(METADATA_START+10, coreParameterCount_onEEPROM);
        eeprom.readByte(METADATA_START+11, wrapperParameterCount_onEEPROM);
        return true;
    }

    bool writeMetadata() {
        SIGNATURE[10] = coreParameterCount;
        SIGNATURE[11] = wrapperParameterCount;
        return eeprom.writeBytes(METADATA_START, SIGNATURE, sizeof(SIGNATURE));
    }

    bool formatBlankDisk() {
        if (!writeCoreDefaults()) return false;
        if (!writeWrapperDefaults()) return false;
        if (!writeMetadata()) return false;

        return true;
    }

    bool writeCoreDefaults() {
        uint16_t address = CORE_START;
        for (uint8_t i = 0; i < coreParameterCount; i++) {
            if (!writeRecord(address, coreParametersBuiltin[i])) return false;
        }
        CORE_END = address;
        return true;
    }

    bool writeWrapperDefaults() { //TODO
        /*uint16_t address = WRAPPER_START;
        for (uint8_t i = 0; i < wrapperParameterCount; i++) {
            if (!writeRecord(address, coreParameters[i])) return false;
        }
        WRAPPER_END = address;*/
        return true;
    }

    void mapParametersFromEEPROM() { //TODO
        // защита от повторного вызова, не исключающая этот вызов
        if (parameters != nullptr) {
            for (uint16_t i = 0; i < parameterCount; i++) {
                delete[] (uint8_t*)parameters[i].value;
                delete[] (uint8_t*)parameters[i].defaultValue;
            }
        }
        delete[] parameters;    
        delete[] memoryMap;

        parameters = nullptr;
        memoryMap = nullptr;
        parameterCount = 0;

        // не нужно, так как значения заполняются в readMetadata()
        //coreParameterCount_onEEPROM = 0;    
        //wrapperParameterCount_onEEPROM = 0;

        //------------------
        
        // считаем полное количество параметров, предполагая, что будут мертвые параметры на EEPROM и/или новые параметры, которых еще нет в EEPROM
        parameterCount = max(coreParameterCount_onEEPROM, coreParameterCount) + max(wrapperParameterCount_onEEPROM, wrapperParameterCount);

        if (parameterCount == 0) return;

        memoryMap = new MemoryIndex[MAX_PARAMETER_COUNT]; //ИСПРАВЛЕНО IMPORTANT TODO: нужно расширить размер до (параметры которые ожидает прошивка + параметры которые есть на EEPROM, но которые не ожидаются прошивкой)
        parameters = new MemoryParameter[MAX_PARAMETER_COUNT];
        
        // ошибка выхода за границу блока игнорируется
        // читаем параметры ядра

        uint16_t cursor = CORE_START;
        uint8_t counter = 0;

        for (uint8_t i = 0; i < coreParameterCount_onEEPROM; i++) {
            bool result = readRecord(cursor, CORE_START + CORE_SIZE, memoryMap[counter]); //курсор сдвигается внутри метода

            if (!result) break;

            memoryMap[counter].isOnEEPROM = true;
            memoryMap[counter].isCoreParameter = true;
            counter++;
        }
        if (counter < coreParameterCount_onEEPROM) {
            //TODO обработка ошибки считывания параметров ядра
        }
        CORE_END = cursor;

        // читаем параметры обёртки

        cursor = WRAPPER_START;
        uint8_t counter2 = 0;

        for (uint8_t i = 0; i < wrapperParameterCount_onEEPROM; i++) {
            bool result = readRecord(cursor, WRAPPER_START + WRAPPER_SIZE, memoryMap[counter+counter2]); //курсор сдвигается внутри метода

            if (!result) break;

            memoryMap[counter+counter2].isOnEEPROM = true;
            memoryMap[counter+counter2].isCoreParameter = false;
            counter2++;
        }
        if (counter2 < wrapperParameterCount_onEEPROM) {
            //TODO обработка ошибки считывания параметров ядра
        }
        WRAPPER_END = cursor;


        //-------------------

        // загружаем в маппинг недостающие параметры
        // сравнение параметров в memoryMap с параметрами в coreParametersBuiltin и wrapperParametersBuiltin
        
        // сравнение и добавление недостающих параметров ядра

        for (uint8_t i_builtin = 0; i_builtin < coreParameterCount; i_builtin++) {
            bool found = false;
            uint8_t builtinCoreParameterNameSize = min(strlen(coreParametersBuiltin[i_builtin].name), (size_t)18);
            for (uint8_t i_eeprom = 0; i_eeprom < counter; i_eeprom++) {
                if (memcmp(
                    memoryMap[i_eeprom].name,
                    coreParametersBuiltin[i_builtin].name,
                    min(memoryMap[i_eeprom].nameSize, builtinCoreParameterNameSize)
                ) == 0) found = true;
            }
            if (found) continue; //если в eeprom есть параметр который ожидает увидеть ядро, продолжаем
            // если искомого параметра нет, нужно добавить и записать
            memoryMap[parameterCount].address = CORE_END; //IMPORTANT TODO: выходим за рамки массива (строка 230)
            memoryMap[parameterCount].flags = encodeFlags(coreParametersBuiltin[i_builtin].type, coreParametersBuiltin[i_builtin].flags, false);
            memoryMap[parameterCount].isCoreParameter = true;
            memoryMap[parameterCount].isOnEEPROM = true; //после выполнения записи
            memoryMap[parameterCount].nameSize = builtinCoreParameterNameSize;
            memoryMap[parameterCount].valueSize = coreParametersBuiltin[i_builtin].valueSize;
            memcpy(memoryMap[parameterCount].name, coreParametersBuiltin[i_builtin].name, builtinCoreParameterNameSize);
            memoryMap[parameterCount].type = coreParametersBuiltin[i_builtin].type;
            uint16_t CORE_END_before = CORE_END;
            writeRecord(CORE_END, coreParametersBuiltin[i_builtin]); //CORE_END увеличивается по мере заполнения новыми записями
            memoryMap[parameterCount].recordSize = CORE_END - CORE_END_before; //размер всей записи на EEPROM
            memoryMap[parameterCount].valueAddress = coreParametersBuiltin[i_builtin].valueAddress; //адрес значения. заполняется в writeRecord
            memoryMap[parameterCount].defaultValueAddress = coreParametersBuiltin[i_builtin].defaultValueAddress; //адрес стандартного значения. заполняется в writeRecord

            parameterCount++;
        }

        //TODO тоже самое но для недостающих параметров обёртки
        

        //-------------------

        // урезаем массив до актуального количества параметров
        MemoryIndex* newMap = new MemoryIndex[parameterCount];
        MemoryParameter* newParameters = new MemoryParameter[parameterCount];

        memcpy(newMap, memoryMap, sizeof(MemoryIndex) * parameterCount);
        memcpy(newParameters, parameters, sizeof(MemoryParameter) * parameterCount);

        delete[] memoryMap;
        delete[] parameters;

        memoryMap = newMap;
        parameters = newParameters;


        // Загружаем содержимое параметров с флагом LOAD_ON_STARTUP.
        for (uint8_t i = 0; i < parameterCount; i++) {
            if (isDead(memoryMap[i].flags)) continue;

            if (shouldLoadOnStartup(memoryMap[i].flags)) {
                loadParameter(i); // игнорируем ошибку считывания
            }
        }
    }

    bool loadParameter(uint16_t index) {
        MemoryParameter& parameter = parameters[index];
        MemoryIndex& info = memoryMap[index];

        if (parameter.loaded) return true;

        uint8_t* value = new uint8_t[info.valueSize];
        uint8_t* defaultValue = new uint8_t[info.valueSize];

        if (!eeprom.readBytes(info.valueAddress, value, info.valueSize)) {
            delete[] value;
            delete[] defaultValue;
            return false;
        }

        if (!eeprom.readBytes(info.defaultValueAddress, defaultValue, info.valueSize)) {
            delete[] value;
            delete[] defaultValue;
            return false;
        }

        parameter.value = value;
        parameter.defaultValue = defaultValue;
        parameter.loaded = true;

        return true;
    }









    bool writeRecord(uint16_t& address, ParameterDefinition& parameter) {
        uint16_t nameSize = strlen(parameter.name);
        if (nameSize > 18) nameSize = 18;

        if (parameter.valueSize == 0) return false;
        if (parameter.defaultValue == nullptr) return false;

        uint16_t recordSize =
            1 +                    // FLAGS
            1 +                    // NAME_SIZE
            nameSize +             // NAME
            2 +                    // VALUE_SIZE
            parameter.valueSize +  // VALUE
            parameter.valueSize;   // DEFAULT_VALUE

        uint8_t* record = new uint8_t[recordSize];

        uint16_t cursor = 0;

        // [FLAGS]
        // [marker: 2bit][isDead: 1bit][type: 4bit][loadOnStartup: 1bit]
        uint8_t flags = encodeFlags(parameter.type, parameter.flags, false);

        record[cursor++] = flags;

        // [NAME_SIZE]
        record[cursor++] = static_cast<uint8_t>(nameSize);

        // [NAME]
        memcpy(&record[cursor], parameter.name, nameSize);
        cursor += nameSize;

        // [VALUE_SIZE]
        record[cursor++] = parameter.valueSize & 0xFF;
        record[cursor++] = parameter.valueSize >> 8;

        // [VALUE]
        memcpy(&record[cursor], parameter.defaultValue, parameter.valueSize);
        parameter.valueAddress = cursor + address;
        cursor += parameter.valueSize;

        // [DEFAULT_VALUE]
        memcpy(&record[cursor], parameter.defaultValue, parameter.valueSize);
        parameter.defaultValueAddress = cursor + address;
        cursor += parameter.valueSize;

        bool result = eeprom.writeBytes(address, record, recordSize);
        delete[] record;
        if (!result) return false;

        address += recordSize;

        return true;
    }

    bool readRecord(uint16_t& address, uint16_t regionLimit, MemoryIndex& index) {
        uint8_t header[2];

        if (!eeprom.readBytes(address, header, 2)) return false;

        uint8_t flags = header[0];
        uint8_t nameSize = header[1];

        if (!isValidFlags(flags)) return false;
        if (nameSize == 0) return false;

        uint16_t cursor = address + 2;

        if (cursor + nameSize + 2 > regionLimit) return false;

        if (!eeprom.readBytes(cursor, (uint8_t*)index.name, nameSize)) return false;

        index.name[nameSize] = '\0';
        cursor += nameSize;

        uint8_t sizeBuffer[2];

        if (!eeprom.readBytes(cursor, sizeBuffer, 2)) return false;

        uint16_t valueSize = sizeBuffer[0] | ((uint16_t)sizeBuffer[1] << 8);

        if (valueSize == 0) return false;

        cursor += 2;

        uint16_t recordSize = 2 + nameSize + 2 + valueSize * 2;

        if (address + recordSize > regionLimit) return false;

        index.address = address;
        index.recordSize = recordSize;

        index.valueAddress = cursor;
        index.defaultValueAddress = cursor + valueSize;

        index.flags = flags;
        index.nameSize = nameSize;
        index.valueSize = valueSize;
        index.type = decodeType(flags);

        return true;
    }

    uint8_t encodeFlags(MemoryType type, uint8_t flags, bool isDead = false) {
        uint8_t result = FLAG_MARKER;

        result |= (static_cast<uint8_t>(type) & 0x0F) << 3; 
        if (isDead)                   result |= FLAG_DEAD;
        if (flags & LOAD_ON_STARTUP)  result |= FLAG_LOAD_ON_STARTUP;

        return result;
    }

    bool isValidFlags(uint8_t flags) {
        return (flags & FLAG_MARKER_MASK) == FLAG_MARKER;
    }

    bool isDead(uint8_t flags) {
        return flags & FLAG_DEAD;
    }

    bool shouldLoadOnStartup(uint8_t flags) {
        return flags & FLAG_LOAD_ON_STARTUP;
    }

    MemoryType decodeType(uint8_t flags) {
        return static_cast<MemoryType>( (flags & FLAG_TYPE_MASK) >> 3 );
    }


    bool raiseError(IOERROR errorType, char* context) {
        Serial.println("--------------------IO ERROR--------------------");
        Serial.print("Error \"");
        Serial.println(errors[(int)errorType]);
        Serial.println("\" in ");
        Serial.print(context);
        Serial.println();
        return false;
    }
};

#endif