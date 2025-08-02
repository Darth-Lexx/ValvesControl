#include "main.h"

// Инициализация времени старта системы
uint32_t systemOverflows = 0;
unsigned long systemStartTime = 0;
unsigned long lastMillisCheck = 0;

//Флаг подачи газа
byte ValveOpen = 0;
//Флаг готовности каналов
byte ChannelReady = 0;
//Флаг опроса каналов
byte NextChannel = 0;

void setError(uint16_t (&data)[17], ErrorGroups errorType, uint8_t device_num, uint8_t error)
{
  // Проверка входных данных
  if (device_num < 1 || device_num > 4)
  {
    return;
  }

  uint8_t bit_offset;
  uint16_t mask;

  switch (errorType)
  {
  case AS200_COMM_ERROR:
    bit_offset = 4 * (4 - device_num);
    mask = 0xF << bit_offset;
    data[6] = (data[6] & ~mask) | ((error & 0xF) << bit_offset);
    break;

  case AFM07_COMM_ERROR:
    bit_offset = 4 * (4 - device_num);
    mask = 0xF << bit_offset;
    data[7] = (data[7] & ~mask) | ((error & 0xF) << bit_offset);
    break;

  case AFM07_INT_ERROR:
    bit_offset = 8 + 2 * (4 - device_num);
    mask = 0x3 << bit_offset;
    data[8] = (data[8] & ~mask) | ((error & 0x3) << bit_offset);
    break;

  case CHANNEL_ERROR:
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

// Функция логгирования с временем и табуляцией
void logMessage(LogLevel level, const String &message)
{
  String timeStr = formatTime();
  String levelStr;
  String colorStart, colorEnd = ANSI_RESET;

  switch (level)
  {
  case LOG_INFO:
    levelStr = "INFO   ";
    colorStart = ANSI_GREEN;
    break;
  case LOG_WARNING:
    levelStr = "WARNING";
    colorStart = ANSI_YELLOW;
    break;
  case LOG_ERROR:
    levelStr = "ERROR  ";
    colorStart = ANSI_RED;
    break;
  case LOG_DEBUG:
    levelStr = "DEBUG  ";
    colorStart = ANSI_CYAN;
    break;
  case LOG_MODBUS: // Специальный цвет для ModBus
    levelStr = "MODBUS ";
    colorStart = ANSI_MAGENTA;
    break;
  default:
    levelStr = "UNKNOWN";
    colorStart = ANSI_WHITE;
  }

  String logEntry = timeStr + "\t" + colorStart + "[" + levelStr + "]" + colorEnd + "\t" + message;
  Serial.println(logEntry);
}

// Функция для форматирования времени в строку [чч:мм:сс.мсс]
String formatTime()
{
  updateTimeTracker();

  // Рассчитываем общее время с учётом переполнений
  uint64_t totalMs = ((uint64_t)systemOverflows << 32) + millis() - systemStartTime;

  // Конвертируем в читаемый формат
  uint32_t seconds = totalMs / 1000;
  uint32_t minutes = seconds / 60;
  uint32_t hours = minutes / 60;
  uint32_t days = hours / 24;

  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  uint32_t ms = totalMs % 1000;

  char timeStr[20];
  snprintf(timeStr, sizeof(timeStr), "[%lud %02lu:%02lu:%02lu.%03lu]",
           days, hours, minutes, seconds, ms);

  return String(timeStr);
}

void updateTimeTracker()
{
  if (millis() < lastMillisCheck)
  {
    systemOverflows++;
  }
  lastMillisCheck = millis();
}

void safeEEPROMWrite() {
    uint8_t bytesWritten = 0;
    EEPROMData currentData;
    
    // 1. Читаем текущие данные
    EEPROM.get(0, currentData);
    
    // 2. Сравниваем и записываем побайтово
    const uint8_t* newData = (uint8_t*)&Data;
    const uint8_t* oldData = (uint8_t*)&currentData;
    
    for (size_t i = 0; i < sizeof(EEPROMData); i++) {
        if (newData[i] != oldData[i]) {
            // 3. Записываем только изменённые байты
            EEPROM.update(i, newData[i]);
            bytesWritten++;
        }
    }
    
if (bytesWritten > 0) {
    char buf[35];
    snprintf(buf, sizeof(buf), "EEPROM: изменено %u из %u байт", 
             bytesWritten, sizeof(EEPROMData));
    logMessage(LOG_INFO, buf);
}
}

void setup()
{
  systemStartTime = millis(); // Засекаем время старта системы
  Serial.begin(115200);
  logMessage(LOG_INFO, "System startup initiated");
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
  logMessage(LOG_INFO, "Pins initialized");

  // открытие клапанов сброса давления
  digitalWrite(PIN_H_REL, PIN_ON);
  digitalWrite(PIN_L_REL, PIN_ON);
  digitalWrite(PIN_N2_REL, PIN_ON);
  logMessage(LOG_INFO, "Pressure relief valves opened");

  delay(2000);

  // закрытие клапанов сброса давления
  digitalWrite(PIN_H_REL, PIN_OFF);
  digitalWrite(PIN_L_REL, PIN_OFF);
  digitalWrite(PIN_N2_REL, PIN_OFF);
  logMessage(LOG_INFO, "Pressure relief valves closed");

  // включение AS200
  digitalWrite(PIN_CTRL1_POWER, PIN_ON);
  digitalWrite(PIN_CTRL2_POWER, PIN_ON);
  digitalWrite(PIN_CTRL3_POWER, PIN_ON);
  digitalWrite(PIN_CTRL4_POWER, PIN_ON);
  logMessage(LOG_INFO, "AS200 controllers powered on");

  // проверка пина сброса настроек
  bool reset = !digitalRead(PIN_RESET);
  // проверка валидности прочитанных данных (сброс если не валидные или установлен пин сброса)
  bool settingsChanged = false;

  if (reset)
  {
    logMessage(LOG_WARNING, "Reset pin is active, settings will be reset");
    Data.ModBusAdr = MBSL_ADR_D;
    Data.ModBusSpeed = 2;
    Data.Delta = MBSL_DELTA_D;
    Data.Flow = MBSL_FLOW_D;
    Data.ChannelsEnable = 15;
    settingsChanged = true;
  }
  else

  { // чтение данных из EEPROM
    EEPROM.get(0, Data);
    logMessage(LOG_INFO, "EEPROM data read");

    if (Data.ModBusAdr == 0 || Data.ModBusAdr > 247)
    {
      Data.ModBusAdr = MBSL_ADR_D;
      settingsChanged = true;
      logMessage(LOG_WARNING, "Reset ModBus address to default: " + String(Data.ModBusAdr));
    }
    if ((Data.ModBusSpeed != 1 && Data.ModBusSpeed != 2 && Data.ModBusSpeed != 4 && Data.ModBusSpeed != 8 && Data.ModBusSpeed != 12 && Data.ModBusSpeed != 24))
    {
      Data.ModBusSpeed = 2;
      settingsChanged = true;
      logMessage(LOG_WARNING, "Reset ModBus speed to default: " + String(Data.ModBusSpeed));
    }
    if (Data.Delta < 30 || Data.Delta > 100)
    {
      Data.Delta = MBSL_DELTA_D;
      settingsChanged = true;
      logMessage(LOG_WARNING, "Reset Delta to default: " + String(Data.Delta));
    }
    if (Data.Flow < 400 || Data.Flow > 1000)
    {
      Data.Flow = MBSL_FLOW_D;
      settingsChanged = true;
      logMessage(LOG_WARNING, "Reset Flow to default: " + String(Data.Flow));
    }
    if (Data.ChannelsEnable > 15)
    {
      Data.ChannelsEnable = 15;
      settingsChanged = true;
      logMessage(LOG_WARNING, "Reset ChannelsEnable to default: " + String(Data.ChannelsEnable));
    }
  }

  if (settingsChanged)
  {
    safeEEPROMWrite();
    logMessage(LOG_INFO, "Settings saved to EEPROM");
  }

  // инициализация порта и модбас связи с ПК
  SlaveSerial.begin(Data.ModBusSpeed * 4800, SERIAL_8N1);
  MbSlave.begin(Data.ModBusAdr, Data.ModBusSpeed * 4800, SERIAL_8N1);
  logMessage(LOG_MODBUS, "ModBus Slave initialized. Address: " + String(Data.ModBusAdr) +
                             ", Speed: " + String(Data.ModBusSpeed * 4800) + " baud");

  // Инициализация связи с AS200 и AFM07
  MasterSerialAS200.begin(38400, SERIAL_8N1);
  AS200Master.begin(38400, SERIAL_8N1);
  MasterSerialAFM07.begin(115200, SERIAL_8N1);
  AFM07Master.begin(115200, SERIAL_8N1);
  logMessage(LOG_MODBUS, "AS200 and AFM07 communication ports initialized");

  for (byte i = 0; i < 4; i++)
  {
    Channels[i]->AS200MbAdr = 1 + i * 2;
    Channels[i]->AFM07MbAdr = 2 + i * 2;
    Channels[i]->setAS200SetFlow(Data.Flow);

    logMessage(LOG_MODBUS, "Channel " + String(i + 1) +
                               " - AS200 address: " + String(Channels[i]->AS200MbAdr) +
                               ", AFM07 address: " + String(Channels[i]->AFM07MbAdr) +
                               ", Set flow: " + String(Data.Flow));
    // Чтение регистров
    Channels[i]->AS200MbError = AS200Master.readHoldingRegisters(Channels[i]->AS200MbAdr, AS200_FLOW_H,
                                                                 Channels[i]->AS200SetFlowArray, array_count(Channels[i]->AS200SetFlowArray));
    Channels[i]->AFM07MbError = AFM07Master.readHoldingRegisters(Channels[i]->AFM07MbAdr, 0,
                                                                 Channels[i]->AFM07Reg, array_count(Channels[i]->AFM07Reg));

    // Проверка ошибок и активация канала
    if (!Channels[i]->AS200MbError && !Channels[i]->AFM07MbError && !Channels[i]->AFM07Reg[AFM07_STATUS])
    {
      if (bitRead(Data.ChannelsEnable, i))
      {
        *Channels[i] = true;
        logMessage(LOG_INFO, "Channel " + String(i + 1) + " activated successfully");
      }
    }
    else
    {
      setError(SlaveRegs, AS200_COMM_ERROR, i + 1, Channels[i]->AS200MbError);
      setError(SlaveRegs, AFM07_COMM_ERROR, i + 1, Channels[i]->AFM07MbError);
      setError(SlaveRegs, AFM07_INT_ERROR, i + 1, Channels[i]->AFM07Reg[AFM07_STATUS]);

      bitClear(SlaveRegs[MBSL_R_CHANNELS], i + 8);
      bitClear(Data.ChannelsEnable, i);
      *Channels[i] = false;

      logMessage(LOG_ERROR, "Channel " + String(i + 1) +
                                " activation failed. AS200 error: " + String(Channels[i]->AS200MbError) +
                                ", AFM07 error: " + String(Channels[i]->AFM07MbError) +
                                ", AFM07 status: " + String(Channels[i]->AFM07Reg[AFM07_STATUS]));
    }
  }

  // инициализация регистров для связи с ПК
  MbSlave.configureHoldingRegisters(SlaveRegs, array_count(SlaveRegs));
  MbSlave.configureInputRegisters(SlaveRegs, array_count(SlaveRegs));
  logMessage(LOG_MODBUS, "ModBus registers configured");

  logMessage(LOG_INFO, "System initialization completed");
}

void loop()
{
  if (MbSlave.poll())
  {
    if (SlaveRegs[MBSL_R_ADR] != Data.ModBusAdr)
    {
      //TODO
    }

    if (SlaveRegs[MBSL_R_SPEED] != Data.ModBusSpeed)
    {
      //TODO
    }

    if (SlaveRegs[MBSL_R_FLOW] != Data.Flow)
    {
      //TODO
    }

    if (SlaveRegs[MBSL_R_DELTA] != Data.Delta)
    {
      //TODO
    }

    if ((SlaveRegs[MBSL_R_CHANNELS] >> 8) != Data.ChannelsEnable)
    {
      //TODO
    }
  }
}