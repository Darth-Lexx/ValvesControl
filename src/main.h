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

#define array_count(a) sizeof(a) / sizeof((a)[0])

//--ModBus slave registers  --//
#define MBSL_R_ADR 0 // rw↓
#define MBSL_R_SPEED 1
#define MBSL_R_FLOW 2
#define MBSL_R_DELTA 3
#define MBSL_R_OC_VALVE 4 // 0 - газ не подается, 1 - высокая смесь, 2 - средняя, 4 - азот
#define MBSL_R_CHANNELS 5 // страший байт - флаги включения каналов, младший - флаги, что расход газа на выходе соответсвует заданному (+- дельта)
#define MBSL_ERROR 6      // флаги по каналам - 1 есть ошибка //ro↓
#define MBSL_ERROR2 7     // по 4 бита на канал. 1 бит - ошибка связи с AS200, 2 - ошибка связи с AFM07, 3 - внутреняя ошибка AFM07, 4 - не удается получить требуемый расход
#define MBSL_R_CH1 8      // фактическое значение канала в см^3 в минуту
#define MBSL_R_CH2 9
#define MBSL_R_CH3 10
#define MBSL_R_CH4 11
#define MBSL_R_CH1_T 12 // температура канала
#define MBSL_R_CH2_T 13
#define MBSL_R_CH3_T 14
#define MBSL_R_CH4_T 15

//--//

//--default values
#define MBSL_ADR_D 3
#define MBSL_SPEED_D 2
#define MBSL_DELTA_D 30
#define MBSL_FLOW_D 600

// //--ModBus slave coils      --//
// #define MBSL_C_H            0       //rw↓
// #define MBSL_C_L            1
// #define MBSL_C_N2           2
// #define MBSL_C_CH1          3
// #define MBSL_C_CH2          4
// #define MBSL_C_CH3          5
// #define MBSL_C_CH4          6
// #define MBSL_C_CH1_READY    7       //ro↓
// #define MBSL_C_CH2_READY    8
// #define MBSL_C_CH3_READY    9
// #define MBSL_C_CH4_READY    10
// //--//

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
#define MasterSerial Serial2

//--Pins                    --//
#define PIN_ON 1
#define PIN_OFF 0

#define PIN_RESET 2
#define PIN_H_VALVE 3
#define PIN_L_VALVE 4
#define PIN_N2_VALVE 5
#define PIN_H_REL 6
#define PIN_L_REL 7
#define PIN_N2_REL 8
#define PIN_CTRL1_POWER 9
#define PIN_CTRL2_POWER 10
#define PIN_CTRL3_POWER 11
#define PIN_CTRL4_POWER 12

#define PIN_BTN_CH1 22
#define PIN_BTN_CH2 23
#define PIN_BTN_CH3 24
#define PIN_BTN_CH4 25

#define PIN_BTN_H 26
#define PIN_BTN_L 27
#define PIN_BTN_N2 28
#define PIN_RE 52
#define PIN_DE 53

ButtonT<PIN_BTN_CH1> Channel1Button;
ButtonT<PIN_BTN_CH2> Channel2Button;
ButtonT<PIN_BTN_CH3> Channel3Button;
ButtonT<PIN_BTN_CH4> Channel4Button;
ButtonT<PIN_BTN_H> HighButton;
ButtonT<PIN_BTN_L> LowButton;
ButtonT<PIN_BTN_N2> N2Button;

ModbusRTUSlave MbSlave(SlaveSerial);
ModbusRTUMaster MbMaster(MasterSerial, PIN_DE, PIN_RE);

uint16_t SlaveRegs[16];
uint16_t SlaveRWRegsActual[2];

struct ChannelStruct
{
    uint16_t AS200SetFlowArray[2];
    float getAS200SetFlow() const
    {
        // uint16_t tmp[2];
        // tmp[0] = AS200SetFlowArray[1];
        // tmp[1] = AS200SetFlowArray[0];
        // return (float &)tmp;

        uint16_t tmpa[2];
        tmpa[0] = AS200SetFlowArray[1];
        tmpa[1] = AS200SetFlowArray[0];
        float tmp;
        memcpy(&tmp, tmpa, sizeof(float));
        return tmp;
    }
    void setAS200SetFlow(float value)
    {
        // uint16_t *ptr = (uint16_t *)&value;
        // AS200SetFlowArray[0] = *(ptr + 1);
        // AS200SetFlowArray[1] = *ptr;
        uint16_t temp[2];
        memcpy(temp, &value, sizeof(value));
        AS200SetFlowArray[0] = temp[1];
        AS200SetFlowArray[1] = temp[0];
    }

    uint16_t AFM07Reg[5];
    float getAFM07Acc() const
    {
        // uint16_t tmp[2];
        // tmp[0] = AFM07Reg[AFM07_ACC_FLOW_L];
        // tmp[1] = AFM07Reg[AFM07_ACC_FLOW_H];
        // return (float &)tmp;
        uint16_t tmpa[2];
        tmpa[0] = AFM07Reg[AFM07_ACC_FLOW_L];
        tmpa[1] = AFM07Reg[AFM07_ACC_FLOW_H];
        float tmp;
        memcpy(&tmp, tmpa, sizeof(float));
        return tmp;
    }
    byte AS200MbAdr;
    byte AFM07MbAdr;
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

EEPROMData Data;

#endif