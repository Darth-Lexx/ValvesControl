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

  // инициализация порта и модбас связи с ПК
  SlaveSerial.begin(Data.ModBusSpeed * 4800, SERIAL_8N1);
  MbSlave.begin(Data.ModBusAdr, Data.ModBusSpeed * 4800, SERIAL_8N1);

  // Инициализация связи с AS200 и AFM07
  for (byte i = 0; i < 4; i++)
  {
    Channels[i]->AS200MbAdr = 1 + i * 2;     // Адреса AS200: 1, 3, 5, 7
    Channels[i]->AFM07MbAdr = 2 + i * 2;     // Адреса AFM07: 2, 4, 6, 8
    Channels[i]->setAS200SetFlow(Data.Flow); // Установка начального расхода для AS200
  }

  MasterSerial.begin(115200, SERIAL_8N1);
  MbMaster.begin(115200, SERIAL_8N1);

  for (byte i = 0; i < 4; i++)
  {
    // Чтение регистров
    Channels[i]->AS200MbError = MbMaster.readHoldingRegisters(Channels[i]->AS200MbAdr, AS200_FLOW_H, Channels[i]->AS200SetFlowArray, array_count(Channels[i]->AS200SetFlowArray));
    Channels[i]->AFM07MbError = MbMaster.readHoldingRegisters(Channels[i]->AFM07MbAdr, 0, Channels[i]->AFM07Reg, array_count(Channels[i]->AFM07Reg));

    // Проверка ошибок и активация канала
    if (!Channels[i]->AS200MbError && !Channels[i]->AFM07MbError && Channels[i]->AFM07Reg[AFM07_STATUS])
    {
      if (bitRead(Data.ChannelsEnable, i))
      {
        *Channels[i] = true; // Используем перегруженный оператор =
      }
    }
    else
    {
      setError(SlaveRegs, AS200_COMM_ERROR, i + 1, Channels[i]->AS200MbError);
      setError(SlaveRegs, AFM07_COMM_ERROR, i + 1, Channels[i]->AFM07MbError);
      setError(SlaveRegs, AFM07_INT_ERROR, i + 1, Channels[i]->AFM07Reg[AFM07_STATUS]);
    }
  }

  // инициализация регистров для связи с ПК
  MbSlave.configureHoldingRegisters(SlaveRegs, array_count(SlaveRegs));
  MbSlave.configureInputRegisters(SlaveRegs, array_count(SlaveRegs));
}
void loop()
{
}

void setError(uint16_t (&data)[17], ErrorGroups errorType, uint8_t device_num, uint8_t error)
{
  // Проверка входных данных
  if (device_num < 1 || device_num > 4)
    return;

  uint8_t bit_offset;
  uint16_t mask;
  uint8_t max_errors;

  switch (errorType)
  {
  case AS200_COMM_ERROR:
    max_errors = 15;
    bit_offset = 4 * (4 - device_num);
    mask = 0xF << bit_offset;
    data[6] = (data[6] & ~mask) | ((error & 0xF) << bit_offset);
    break;

  case AFM07_COMM_ERROR:
    max_errors = 15;
    bit_offset = 4 * (4 - device_num);
    mask = 0xF << bit_offset;
    data[7] = (data[7] & ~mask) | ((error & 0xF) << bit_offset);
    break;

  case AFM07_INT_ERROR:
    max_errors = 3;
    bit_offset = 8 + 2 * (4 - device_num);
    mask = 0x3 << bit_offset;
    data[8] = (data[8] & ~mask) | ((error & 0x3) << bit_offset);
    break;

  case CHANNEL_ERROR:
    max_errors = 3;
    bit_offset = 2 * (4 - device_num);
    mask = 0x3 << bit_offset;
    data[8] = (data[8] & ~mask) | ((error & 0x3) << bit_offset);
    break;
  }
}

uint8_t getError(const uint16_t (&data)[17], ErrorGroups errorType, uint8_t device_num)
{
  if (device_num < 1 || device_num > 4)
    return 0;

  switch (errorType)
  {
  case AS200_COMM_ERROR:
    return (data[6] >> (4 * (4 - device_num))) & 0xF;

  case AFM07_COMM_ERROR:
    return (data[7] >> (4 * (4 - device_num))) & 0xF;

  case AFM07_INT_ERROR:
    return (data[8] >> (8 + 2 * (4 - device_num))) & 0x3;

  case CHANNEL_ERROR:
    return (data[8] >> (2 * (4 - device_num))) & 0x3;

  default:
    return 0;
  }
}
