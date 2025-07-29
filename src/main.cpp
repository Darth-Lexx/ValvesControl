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
  SlaveMbAdr = EEPROM.read(EEPROM_ADR);
  SlaveMbSpeed = EEPROM.read(EEPROM_SPEED);
  Delta = EEPROM.read(EEPROM_DELTA);
  EEPROM.get(EEPROM_FLOW, Flow);
  ChannelsEnable = EEPROM.read(EEPROM_CH_ENABLE);

  // проверка валидности прочитанных данных (сброс если не валидные или установлен пин сброса)
  if (SlaveMbAdr == 0 || SlaveMbAdr > 247 || reset)
  {
    SlaveMbAdr = MBSL_ADR_D;
    EEPROM.update(EEPROM_ADR, SlaveMbAdr);
  }
  if ((SlaveMbSpeed != 1 && SlaveMbSpeed != 2 && SlaveMbSpeed != 4 && SlaveMbSpeed != 8 && SlaveMbSpeed != 12 && SlaveMbSpeed != 24) || reset)
  {
    SlaveMbSpeed = 2;
    EEPROM.update(EEPROM_SPEED, SlaveMbSpeed);
  }
  if (Delta < 30 || Delta > 100 || reset)
  {
    Delta = MBSL_DELTA_D;
    EEPROM.update(EEPROM_DELTA, Delta);
  }
  if (Flow < 400 || Flow > 1000 || reset)
  {
    Flow = MBSL_FLOW_D;
    EEPROM.put(EEPROM_FLOW, Flow);
  }
  if (ChannelsEnable > 15 || reset)
  {
    ChannelsEnable = 15;
    EEPROM.update(EEPROM_CH_ENABLE, ChannelsEnable);
  }
}

void loop()
{
}