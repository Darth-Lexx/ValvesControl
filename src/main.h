#ifndef ValvesControl_H
#define ValvesControl_H

#include <Arduino.h>

#define EB_NO_FOR           // отключить поддержку pressFor/holdFor/stepFor и счётчик степов (экономит 2 байта оперативки)
#define EB_NO_CALLBACK      // отключить обработчик событий attach (экономит 2 байта оперативки)
#define EB_NO_COUNTER       // отключить счётчик энкодера (экономит 4 байта оперативки)
#define EB_NO_BUFFER        // отключить буферизацию энкодера (экономит 1 байт оперативки)
#define EB_DEB_TIME     50      // таймаут гашения дребезга кнопки (кнопка)
#define EB_CLICK_TIME   200   // таймаут ожидания кликов (кнопка)
#define EB_HOLD_TIME    300    // таймаут удержания (кнопка)
#define EB_STEP_TIME    500
#include <EncButton.h>

#include <ModbusRTUSlave.h>
#include <ModbusRTUMaster.h>

#include <EEPROM.h>

#define array_count(a) sizeof(a) / sizeof((a)[0])

//--ModBus slave registers  --//
#define MBSL_R_ADR          0       //rw
#define MBSL_R_SPEED        1
#define MBSL_R_FLOW         2
#define MBSL_R_DELTA        3
#define MBSL_R_CH1_VH       4       //ro
#define MBSL_R_CH1_VL       5
#define MBSL_R_CH2_VH       6
#define MBSL_R_CH2_VL       7
#define MBSL_R_CH3_VH       8
#define MBSL_R_CH3_VL       9
#define MBSL_R_CH4_VH       10
#define MBSL_R_CH4_VL       11
#define MBSL_ERROR          12      //старший байт - флаги каналов, младший байт - 0 и 1 биты - флаги ошибки первого канала
//--//

//--default values
#define MBSL_ADR_D          3
#define MBSL_SPEED_D        2
#define MBSL_DELTA_D        30
#define MBSL_FLOW_D         600

//--ModBus slave coils      --//
#define MBSL_C_H           0       //rw
#define MBSL_C_L           1
#define MBSL_C_N2           2
#define MBSL_C_CH1          3
#define MBSL_C_CH2          4
#define MBSL_C_CH3          5
#define MBSL_C_CH4          6
#define MBSL_C_CH1_READY    7       //ro
#define MBSL_C_CH2_READY    8
#define MBSL_C_CH3_READY    9
#define MBSL_C_CH4_READY    10
//--//

#define SlaveSerial Serial1
#define MasterSerial Serial2

//--Pins                    --//
#define PIN_ON              1
#define PIN_OFF             0

#define PIN_RESET           2
#define PIN_H_VALVE         3
#define PIN_L_VALVE         4
#define PIN_N2_VALVE        5
#define PIN_H_REL           6
#define PIN_L_REL           7
#define PIN_N2_REL          8
#define PIN_CTRL1_POWER     9
#define PIN_CTRL2_POWER     10
#define PIN_CTRL3_POWER     11
#define PIN_CTRL4_POWER     12

#define PIN_BTN_CH1         22
#define PIN_BTN_CH2         23
#define PIN_BTN_CH3         24
#define PIN_BTN_CH4         25

#define PIN_BTN_H           26
#define PIN_BTN_L           27
#define PIN_BTN_N2          28
#define PIN_DE              53

//--EEPROM                  --//
#define EEPROM_ADR          0
#define EEPROM_SPEED        1
#define EEPROM_FLOW         2 //float - 4b
#define EEPROM_DELTA        6
#define EEPROM_CH_ENABLE    7

ButtonT<PIN_BTN_CH1> Channel1Button;
ButtonT<PIN_BTN_CH2> Channel2Button;
ButtonT<PIN_BTN_CH3> Channel3Button;
ButtonT<PIN_BTN_CH4> Channel4Button;
ButtonT<PIN_BTN_H> HighButton;
ButtonT<PIN_BTN_L> LowButton;
ButtonT<PIN_BTN_N2> N2Button;

ModbusRTUSlave MbSlave(SlaveSerial);
ModbusRTUMaster MbMaster(MasterSerial, PIN_DE);

byte SlaveMbAdr, SlaveMbSpeed, Delta, ChannelsEnable;
float Flow;

#endif