#ifndef ValvesControl_H
#define ValvesControl_H

#include <Arduino.h>

#define EB_NO_FOR         // отключить поддержку pressFor/holdFor/stepFor и счётчик степов (экономит 2 байта оперативки)
#define EB_NO_CALLBACK    // отключить обработчик событий attach (экономит 2 байта оперативки)
#define EB_NO_COUNTER     // отключить счётчик энкодера (экономит 4 байта оперативки)
#define EB_NO_BUFFER      // отключить буферизацию энкодера (экономит 1 байт оперативки)
#define EB_DEB_TIME 50    // таймаут гашения дребезга кнопки (кнопка)
#define EB_CLICK_TIME 200 // таймаут ожидания кликов (кнопка)
#define EB_HOLD_TIME 300  // таймаут удержания (кнопка)
#define EB_STEP_TIME 500
#include <EncButton.h>

#include <ModbusRTUSlave.h>
#include <ModbusRTUMaster.h>

#include <EEPROM.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "font.h"

// Настройки дисплея
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_MOSI  51  // Фиксированный SPI
#define OLED_CLK   52  // Фиксированный SPI
#define OLED_DC    8   // Любой цифровой
#define OLED_CS    53  // Рекомендуется 53
#define OLED_RESET 9   // Любой цифровой

#define ANSI_COLORS  // Раскомментировать для включения цветного вывода

// В секцию enum LogLevel добавляем:
enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR, 
    LOG_DEBUG,
    LOG_MODBUS  // Новый тип для ModBus-сообщений
};

// ========== main.cpp ==========
// Добавляем в начало файла:
#ifdef ANSI_COLORS
// ANSI Color Codes
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#else
// Пустые define для отключения цветов
#define ANSI_RESET   ""
#define ANSI_RED     ""
#define ANSI_GREEN   ""
#define ANSI_YELLOW  ""
#define ANSI_BLUE    ""
#define ANSI_MAGENTA ""
#define ANSI_CYAN    ""
#define ANSI_WHITE   ""
#endif

// Прототипы функций логгирования
void logMessage(LogLevel level, const String& message);
void updateTimeTracker();
String formatTime();

// Глобальная переменная для времени старта системы
extern uint32_t systemOverflows;
extern unsigned long systemStartTime;

#define array_count(a) sizeof(a) / sizeof((a)[0])

//--ModBus slave registers  --//
#define MBSL_R_ADR 0 // rw↓
#define MBSL_R_SPEED 1
#define MBSL_R_FLOW 2
#define MBSL_R_DELTA 3
#define MBSL_R_OC_VALVE 4 // 0 - газ не подается, 1 - высокая смесь, 2 - средняя, 4 - азот
#define MBSL_R_CHANNELS 5 // страший байт - флаги включения каналов, младший - флаги, что расход газа на выходе соответсвует заданному (+- дельта)
#define MBSL_ERROR 6      // ro↓
#define MBSL_ERROR2 7
#define MBSL_ERROR3 8
// 48 бит:
// 0-3 ошибка связи AS200-1
// 4-7 -//- AS200-2
// 8-11 -//- AS200-3
// 12-15 -//- AS200-4
// 16-19 -//- AFM07-1
// 20-23 -//- AFM07-2
// 24-27 -//- AFM07-3
// 28-31 -//- AFM07-4
// 32-33 - внутренняя ошибка AFM07-1
// 34-35 - внутренняя ошибка AFM07-2
// 36-37 - внутренняя ошибка AFM07-3
// 38-39 - внутренняя ошибка AFM07-4
// 40-41 - ошибка расхода первого канала - 1 - раход = 0, 2 - лимит регулирования, 3 - на выходе больше, чем на входе.
// 42-43 - -//- второго канала
// 44-45 - -//- третьего канала
// 46-47 - -//- четвёртого канала
#define MBSL_R_CH1 9 // фактическое значение канала в см^3 в минуту
#define MBSL_R_CH2 10
#define MBSL_R_CH3 11
#define MBSL_R_CH4 12
#define MBSL_R_CH1_T 13 // температура канала
#define MBSL_R_CH2_T 14
#define MBSL_R_CH3_T 15
#define MBSL_R_CH4_T 16

//--//

//--default values
#define MBSL_ADR_D 3
#define MBSL_SPEED_D 2
#define MBSL_DELTA_D 30
#define MBSL_FLOW_D 600

//--ModBus AS200 registers  --//
#define AS200_FLOW_H 0 // float ro
#define AS200_FLOW_L 1
#define AS200_SET_FLOW_H 2 // float rw
#define AS200_SET_FLOW_L 3
#define AS200_ACC_FLOW_1 4 // 64bit double rw (1 - for reset)
#define AS200_ACC_FLOW_2 5
#define AS200_ACC_FLOW_3 6
#define AS200_ACC_FLOW_4 7
//--//

//--ModBus AFM07 registers  --//
#define AFM07_FLOW 0       // in cm^3 (2 l/m - 2000 cm^3/min)
#define AFM07_ACC_FLOW_H 1 // float
#define AFM07_ACC_FLOW_L 2
#define AFM07_TEMP 3       //(T*10)
#define AFM07_STATUS 4     // 0 - normal, 1 - sensor error, 2 - EEPROM error, 3 - both
#define AFM07_ACC_RESET 55 // 1 for reset
//--//

#define SlaveSerial Serial1
#define MasterSerialAS200 Serial2
#define MasterSerialAFM07 Serial3

//--Pins                    --//
#define PIN_ON 1
#define PIN_OFF 0

#define PIN_RESET 2
#define PIN_H_VALVE 3
#define PIN_L_VALVE 4
#define PIN_N2_VALVE 5
#define PIN_H_REL 6
#define PIN_L_REL 7
#define PIN_N2_REL 38
#define PIN_CTRL1_POWER 39
#define PIN_CTRL2_POWER 40
#define PIN_CTRL3_POWER 11
#define PIN_CTRL4_POWER 12

#define PIN_BTN_CH1 22
#define PIN_BTN_CH2 23
#define PIN_BTN_CH3 24
#define PIN_BTN_CH4 25

#define PIN_BTN_H 26
#define PIN_BTN_L 27
#define PIN_BTN_N2 28
#define PIN_RE_AS200 52
#define PIN_DE_AS200 53
#define PIN_RE_AFM07 50
#define PIN_DE_AFM07 51

ButtonT<PIN_BTN_CH1> Channel1Button;
ButtonT<PIN_BTN_CH2> Channel2Button;
ButtonT<PIN_BTN_CH3> Channel3Button;
ButtonT<PIN_BTN_CH4> Channel4Button;
ButtonT<PIN_BTN_H> HighButton;
ButtonT<PIN_BTN_L> LowButton;
ButtonT<PIN_BTN_N2> N2Button;

ModbusRTUSlave MbSlave(SlaveSerial);
ModbusRTUMaster AS200Master(MasterSerialAS200, PIN_RE_AS200, PIN_DE_AS200);
ModbusRTUMaster AFM07Master(MasterSerialAFM07, PIN_RE_AFM07, PIN_DE_AFM07);

uint16_t SlaveRegs[17];
uint16_t SlaveRWRegsActual[2];

union FloatConverter {
    float asFloat;
    uint16_t asWords[2];
};

struct ChannelStruct
{
    uint16_t AS200SetFlowArray[2] = {0, 0};
    float getAS200SetFlow() const
    {
        FloatConverter converter;
        converter.asWords[0] = AS200SetFlowArray[1];
        converter.asWords[1] = AS200SetFlowArray[0];
        return converter.asFloat;
    }
    void setAS200SetFlow(float value)
    {
        FloatConverter converter;
        converter.asFloat = value;
        AS200SetFlowArray[0] = converter.asWords[1];
        AS200SetFlowArray[1] = converter.asWords[0];
    }

    uint16_t AFM07Reg[5] = { 0,0,0,0,0};
    float getAFM07Acc() const
    {
        FloatConverter converter;
        converter.asWords[0] = AFM07Reg[AFM07_ACC_FLOW_L];
        converter.asWords[1] = AFM07Reg[AFM07_ACC_FLOW_H];
        return converter.asFloat;
    }
    byte AS200MbAdr = 1;
    byte AFM07MbAdr = 1;
    byte AS200MbError = 0;
    byte AFM07MbError = 0;
    byte AFM07OffSet = 0;

    operator bool() const
    {
        return Enabled;
    }

    ChannelStruct &operator=(bool value)
    {
        Enabled = value;
        return *this;
    }

    bool operator!() const
    {
        return !Enabled;
    }

private:
    bool Enabled = false; // флаг включения канала
};

struct EEPROMData
{
    byte ModBusAdr = MBSL_ADR_D;
    byte ModBusSpeed = MBSL_SPEED_D;
    byte Delta = MBSL_DELTA_D;
    byte ChannelsEnable = 15;
    uint16_t Flow = MBSL_FLOW_D;
};

ChannelStruct Channel1Data;
ChannelStruct Channel2Data;
ChannelStruct Channel3Data;
ChannelStruct Channel4Data;
ChannelStruct* Channels[] = { &Channel1Data, &Channel2Data, &Channel3Data, &Channel4Data };

EEPROMData Data;

enum ErrorGroups
{
    AS200_COMM_ERROR, 
    AFM07_COMM_ERROR, 
    AFM07_INT_ERROR, 
    CHANNEL_ERROR 
};
void setError(uint16_t (&data)[17], ErrorGroups errorType, uint8_t device_num, uint8_t error);
uint8_t getError(const uint16_t (&data)[17], ErrorGroups errorType, uint8_t device_num);

#endif