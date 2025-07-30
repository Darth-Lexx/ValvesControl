#include "main.h"

void setup()
{
  Serial.begin(115200);
  // установка пинов ресета, кнопок и реле
  pinMode(PIN_RESET, INPUT_PULLUP);
  pinMode(PIN_BTN_CH1, INPUT_PULLUP);
  pinMode(PIN_BTN_CH2, INPUT_PULLUP);
  pinMode(PIN_BTN_CH3, INPUT_PULLUP);
  pinMode(PIN_BTN_CH4, INPUT_PULLUP);
  pinMode(PIN_BTN_H, INPUT_PULLUP);
  pinMode(PIN_BTN_L, INPUT_PULLUP);
  pinMode(PIN_BTN_N2, INPUT_PULLUP);

  pinMode(PIN_H_VALVE, OUTPUT);
  pinMode(PIN_L_VALVE, OUTPUT);
  pinMode(PIN_N2_VALVE, OUTPUT);
  pinMode(PIN_H_REL, OUTPUT);
  pinMode(PIN_L_REL, OUTPUT);
  pinMode(PIN_N2_REL, OUTPUT);
  pinMode(PIN_CTRL1_POWER, OUTPUT);
  pinMode(PIN_CTRL2_POWER, OUTPUT);
  pinMode(PIN_CTRL3_POWER, OUTPUT);
  pinMode(PIN_CTRL4_POWER, OUTPUT);
  digitalWrite(PIN_H_VALVE, PIN_OFF);
  digitalWrite(PIN_L_VALVE, PIN_OFF);
  digitalWrite(PIN_N2_VALVE, PIN_OFF);
  digitalWrite(PIN_CTRL1_POWER, PIN_OFF);
  digitalWrite(PIN_CTRL2_POWER, PIN_OFF);
  digitalWrite(PIN_CTRL3_POWER, PIN_OFF);
  digitalWrite(PIN_CTRL4_POWER, PIN_OFF);

  // открытие клапанов сброса давления
  digitalWrite(PIN_H_REL, PIN_ON);
  digitalWrite(PIN_L_REL, PIN_ON);
  digitalWrite(PIN_N2_REL, PIN_ON);

  delay(2000);

  // закрытие клапанов сброса давления
  digitalWrite(PIN_H_REL, PIN_OFF);
  digitalWrite(PIN_L_REL, PIN_OFF);
  digitalWrite(PIN_N2_REL, PIN_OFF);

  // включение AS200
  digitalWrite(PIN_CTRL1_POWER, PIN_ON);
  digitalWrite(PIN_CTRL2_POWER, PIN_ON);
  digitalWrite(PIN_CTRL3_POWER, PIN_ON);
  digitalWrite(PIN_CTRL4_POWER, PIN_ON);

  // проверка пина сброса настроек
  bool reset = !digitalRead(PIN_RESET);
  // чтение данных из EEPROM
  EEPROM.get(0, Data);

  // проверка валидности прочитанных данных (сброс если не валидные или установлен пин сброса)
  if (Data.ModBusAdr == 0 || Data.ModBusAdr > 247 || reset)
  {
    Data.ModBusAdr = MBSL_ADR_D;
    EEPROM.put(0, Data);
  }
  if ((Data.ModBusSpeed != 1 && Data.ModBusSpeed != 2 && Data.ModBusSpeed != 4 && Data.ModBusSpeed != 8 && Data.ModBusSpeed != 12 && Data.ModBusSpeed != 24) || reset)
  {
    Data.ModBusSpeed = 2;
    EEPROM.put(0, Data);
  }
  if (Data.Delta < 30 || Data.Delta > 100 || reset)
  {
    Data.Delta = MBSL_DELTA_D;
    EEPROM.put(0, Data);
  }
  if (Data.Flow < 400 || Data.Flow > 1000 || reset)
  {
    Data.Flow = MBSL_FLOW_D;
    EEPROM.put(0, Data);
  }
  if (Data.ChannelsEnable > 15 || reset)
  {
    Data.ChannelsEnable = 15;
    EEPROM.put(0, Data);
  }

  //инициализация регистров для связи с ПК
  MbSlave.configureHoldingRegisters(SlaveRegs, array_count(SlaveRegs));
  MbSlave.configureInputRegisters(SlaveRegs, array_count(SlaveRegs));

  //инициализация порта и модбас связи с ПК
  SlaveSerial.begin(Data.ModBusSpeed * 4800, SERIAL_8N1);
  MbSlave.begin(Data.ModBusAdr, Data.ModBusSpeed * 4800, SERIAL_8N1);

  //Инициализация связи с AS200 и AFM07
  Channel1Data.AS200MbAdr = 1;
  Channel2Data.AS200MbAdr = 3;
  Channel3Data.AS200MbAdr = 5;
  Channel4Data.AS200MbAdr = 7;
  Channel1Data.AFM07MbAdr = 2;
  Channel2Data.AFM07MbAdr = 4;
  Channel3Data.AFM07MbAdr = 6;
  Channel4Data.AFM07MbAdr = 8;
  Channel1Data.setAS200SetFlow(Data.Flow);
  Channel2Data.setAS200SetFlow(Data.Flow);
  Channel3Data.setAS200SetFlow(Data.Flow);
  Channel4Data.setAS200SetFlow(Data.Flow);

  MasterSerial.begin(115200, SERIAL_8N1);
  MbMaster.begin(115200, SERIAL_8N1);

  
}
void loop()
{
}