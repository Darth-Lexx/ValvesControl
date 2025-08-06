#pragma once
#ifndef ValvesControl_MAIN_H
#define ValvesControl_MAIN_H

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

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "font.h"

// Настройки дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI 51 // Фиксированный SPI
#define OLED_CLK 52  // Фиксированный SPI
#define OLED_DC 8    // Любой цифровой
#define OLED_CS 53   // Рекомендуется 53
#define OLED_RESET 9 // Любой цифровой

#define TOP_TEXT_SP 1, 0
#define BOTTOM_RECR_TL 0, 57
#define CH1_TL 0, 10
#define CH1_BR 62, 31
#define CH3_TL 0, 33
#define CH3_BR 62, 54
#define CH2_TL 65, 10
#define CH2_BR 127, 31
#define CH4_TL 65, 33
#define CH4_BR 127, 54

#define CH1_STR2 0, 21
#define CH2_STR2 65, 21
#define CH3_STR2 0, 44
#define CH4_STR2 65, 44

#define CH1_STR2_CHECK 34, 27
#define CH2_STR2_CHECK 99, 27
#define CH3_STR2_CHECK 34, 52
#define CH4_STR2_CHECK 99, 52

#define CH1_STR2_STR2_CLEAR 0, 18, 31, 11
#define CH2_STR2_STR2_CLEAR 65, 18, 31, 11
#define CH3_STR2_STR2_CLEAR 0, 43, 31, 11
#define CH4_STR2_STR2_CLEAR 65, 43, 31, 11

#define CH1_STR2_CHECK_CLEAR 24, 18, 31, 11
#define CH2_STR2_CHECK_CLEAR 89, 18, 31, 11
#define CH3_STR2_CHECK_CLEAR 24, 43, 31, 11
#define CH4_STR2_CHECK_CLEAR 89, 43, 31, 11

#define CH_H 22
#define CH_W 63

// дисплей
extern Adafruit_SSD1306 display;

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

#define PIN_RE_AS200 44
#define PIN_DE_AS200 45
#define PIN_RE_AFM07 42
#define PIN_DE_AFM07 43

extern ModbusRTUSlave MbSlave;
extern ModbusRTUMaster AS200Master;
extern ModbusRTUMaster AFM07Master;

extern uint16_t SlaveRegs[17];
extern uint16_t SlaveRWRegsActual[2];

union FloatConverter
{
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

    uint16_t AFM07Reg[5] = {0, 0, 0, 0, 0};
    float getAFM07Acc() const
    {
        FloatConverter converter;
        converter.asWords[0] = AFM07Reg[AFM07_ACC_FLOW_L];
        converter.asWords[1] = AFM07Reg[AFM07_ACC_FLOW_H];
        return converter.asFloat;
    }
    byte AS200MbAdr = 1;
    byte AFM07MbAdr = 2;
    byte AS200MbError = 0;
    byte AFM07MbError = 0;
    byte AFM07OffSet = 0;
    bool FlowStabilized = false;
    bool IsFlowSet = false;

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

extern ChannelStruct Channel1Data;
extern ChannelStruct Channel2Data;
extern ChannelStruct Channel3Data;
extern ChannelStruct Channel4Data;
extern ChannelStruct *Channels[4];

extern EEPROMData Data;

enum ErrorGroups
{
    AS200_COMM_ERROR,
    AFM07_COMM_ERROR,
    AFM07_INT_ERROR,
    CHANNEL_ERROR
};

#endif