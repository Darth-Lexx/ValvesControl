#include "main.h"
#include "logs.h"
#include "common.h"
#include "flow_dispatcher.h"

// Инициализация времени старта системы
uint32_t systemOverflows = 0;
unsigned long systemStartTime = 0;
unsigned long lastMillisCheck = 0;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

ModbusRTUSlave MbSlave(SlaveSerial);
ModbusRTUMaster AS200Master(MasterSerialAS200, PIN_DE_AS200);
ModbusRTUMaster AFM07Master(MasterSerialAFM07, PIN_DE_AFM07);

uint16_t SlaveRegs[17] = {0};
uint16_t SlaveRWRegsActual[2] = {0};

// Флаг подачи газа
byte ValveOpen = 0;
byte CurrentChannel = 0;

// флаги дисплея
// флаг перерисовки верхней строки
bool TopStringUpdate = false;

ChannelStruct Channel1Data;
ChannelStruct Channel2Data;
ChannelStruct Channel3Data;
ChannelStruct Channel4Data;
ChannelStruct *Channels[4] = {&Channel1Data, &Channel2Data, &Channel3Data, &Channel4Data};

EEPROMData Data;

uTimer16<millis> relTimer;


void setup()
{
  (void)EEPROM;
  systemStartTime = millis(); // Засекаем время старта системы
  Serial.begin(115200);
  logMessage(LOG_INFO, "System startup initiated");

  // Инициализация дисплея
  if (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    logMessage(LOG_WARNING, "Ошибка инициализации SSD1309");
  }
  // Очистка буфера
  display.clearDisplay();

  // Вывод текста
  display.setFont(NULL);
  display.setTextColor(WHITE);
  display.setCursor(0, 21);
  display.print("Initialization...");
  display.display();

  // установка пинов ресета, кнопок и реле
  pinInit();

  display.print(".");
  display.display();
  logMessage(LOG_INFO, "Pins initialized");

  // открытие клапана сброса давления
  digitalWrite(PIN_REL_VALVE, PIN_ON);

  display.print(".");
  display.display();
  logMessage(LOG_INFO, "Pressure relief valves opened");

  delay(2000);

  // закрытие клапанов сброса давления
  digitalWrite(PIN_REL_VALVE, PIN_OFF);

  display.print(".");
  display.display();
  logMessage(LOG_INFO, "Pressure relief valves closed");

  // включение AS200
  delay(3000);
  digitalWrite(PIN_CTRL1_POWER, PIN_ON);
  delay(3000);
  digitalWrite(PIN_CTRL2_POWER, PIN_ON);
  delay(3000);
  digitalWrite(PIN_CTRL3_POWER, PIN_ON);
  delay(3000);
  digitalWrite(PIN_CTRL4_POWER, PIN_ON);
  delay(3000);

  display.print(".");
  display.display();
  logMessage(LOG_INFO, "AS200 controllers powered on");

  // проверка reset/чтение EEPROM
  startSettings();

  // UART init
  UARTinit();

  // первый опрос AS200 и AFM07
  AS200AFM07Init();

  // инициализация регистров для связи с ПК
  MbSlave.configureHoldingRegisters(SlaveRegs, array_count(SlaveRegs));
  MbSlave.configureInputRegisters(SlaveRegs, array_count(SlaveRegs));

  display.print(".");
  display.display();
  logMessage(LOG_MODBUS, "ModBus registers configured");

  logMessage(LOG_INFO, "System initialization completed");

  // первичная разметка дисплея
  displayDrawNormal();
}

void loop()
{
  // обработка запроса по модбас от ПК
  SlavePoll();

  ChannelSurvey();

  if (relTimer.timeout(1000))
  {
    digitalWrite(PIN_REL_VALVE, PIN_OFF);
    relTimer.stop();
  }

  processFlowQueue();
}

void ChannelSurvey()
{

  if (Channels[CurrentChannel])
  {
    Channels[CurrentChannel]->AFM07MbError = AFM07Master.readHoldingRegisters(Channels[CurrentChannel]->AFM07MbAdr, AFM07_FLOW, Channels[CurrentChannel]->AFM07Reg, sizeof(Channels[CurrentChannel]->AFM07Reg));
    Channels[CurrentChannel]->AS200MbError = AS200Master.readHoldingRegisters(Channels[CurrentChannel]->AS200MbAdr, AS200_FLOW_H, Channels[CurrentChannel]->AS200ActualFlowArray, sizeof(Channels[CurrentChannel]->AS200ActualFlowArray));

    if (!Channels[CurrentChannel]->getIsFlowSet())
    {
      logMessage(LOG_INFO, "IsFlowSet не установлено в true, повторная попытка отправки команды для канала " + String(CurrentChannel + 1));

      // повторно выставляем запрос на отправку SetFlow
      requestFlowSend(CurrentChannel);
      bitClear(SlaveRegs[MBSL_R_CHANNELS], CurrentChannel);

      // на этом шаге НИЧЕГО не отключаем, логику автоотключения
      // можно будет сделать позже через счётчик попыток / ошибки ModBus
    }

    unsigned long t = millis() - Channels[CurrentChannel]->time;

    if (!Channels[CurrentChannel]->FlowStabilized && Channels[CurrentChannel]->getAS200SetFlow() > 0)
    {
      if (t > 1000)
      {
        ///////////////!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      }
    }
  }
  CurrentChannel++;
  if (CurrentChannel > 3)
    CurrentChannel = 0;
}

void ValveSetOff()
{
  digitalWrite(PIN_L_VALVE, PIN_OFF);
  digitalWrite(PIN_H_VALVE, PIN_OFF);
  digitalWrite(PIN_N2_VALVE, PIN_OFF);

  for (size_t i = 0; i < 4; i++)
  {
    Channels[i]->setAS200SetFlow(0);
    Channels[i]->setIsFlowSet(false);
  }

  digitalWrite(PIN_REL_VALVE, PIN_ON);
  relTimer.start();
  ValveOpen = 0;
  clearProgressBar();
}

void ValveSetH()
{
  digitalWrite(PIN_L_VALVE, PIN_OFF);
  digitalWrite(PIN_N2_VALVE, PIN_OFF);
  digitalWrite(PIN_H_VALVE, PIN_ON);

  for (size_t i = 0; i < 4; i++)
  {
    if (!Channels[i] || Channels[i]->getIsFlowSet())
      continue;
    Channels[i]->setAS200SetFlow(Data.Flow);
    Channels[i]->setIsFlowSet(false);
  }

  ValveOpen = 1;
  progressBarTime = millis();
}

void ValveSetL()
{
  digitalWrite(PIN_N2_VALVE, PIN_OFF);
  digitalWrite(PIN_H_VALVE, PIN_OFF);
  digitalWrite(PIN_L_VALVE, PIN_ON);

  for (size_t i = 0; i < 4; i++)
  {
    if (!Channels[i] || Channels[i]->getIsFlowSet())
      continue;
    Channels[i]->setAS200SetFlow(Data.Flow);
    Channels[i]->setIsFlowSet(false);
  }

  ValveOpen = 2;
  progressBarTime = millis();
}

void ValveSetN2()
{
  digitalWrite(PIN_L_VALVE, PIN_OFF);
  digitalWrite(PIN_H_VALVE, PIN_OFF);
  digitalWrite(PIN_N2_VALVE, PIN_ON);

  for (size_t i = 0; i < 4; i++)
  {
    if (!Channels[i] || Channels[i]->getIsFlowSet())
      continue;
    Channels[i]->setAS200SetFlow(Data.Flow);
    Channels[i]->setIsFlowSet(false);
  }

  ValveOpen = 3;
  progressBarTime = millis();
}

void SlavePoll()
{
  if (MbSlave.poll())
  {
    bool mbChanged = false;
    bool flowChanged = false;
    bool channelsChanged = false;
    bool valveChanged = false;

    if (SlaveRegs[MBSL_R_ADR] != Data.ModBusAdr && SlaveRegs[MBSL_R_ADR] != 0 && SlaveRegs[MBSL_R_ADR] < 248)
    {
      mbChanged = true;
      TopStringUpdate = true;
      Data.ModBusAdr = (byte)SlaveRegs[MBSL_R_ADR];
      logMessage(LOG_INFO, "Получен новый адрес ModBus");
    }

    if (SlaveRegs[MBSL_R_SPEED] != Data.ModBusSpeed && (SlaveRegs[MBSL_R_SPEED] != 1 && SlaveRegs[MBSL_R_SPEED] != 2 && SlaveRegs[MBSL_R_SPEED] != 4 && SlaveRegs[MBSL_R_SPEED] != 8 && SlaveRegs[MBSL_R_SPEED] != 12 && SlaveRegs[MBSL_R_SPEED] != 24))
    {
      mbChanged = true;
      TopStringUpdate = true;
      Data.ModBusSpeed = (byte)SlaveRegs[MBSL_R_SPEED];
      logMessage(LOG_INFO, "Получена новая скорость ModBus");
    }

    if (SlaveRegs[MBSL_R_FLOW] != Data.Flow && SlaveRegs[MBSL_R_FLOW] > 399 && SlaveRegs[MBSL_R_FLOW] < 1001)
    {
      TopStringUpdate = true;
      flowChanged = true;
      Data.Flow = SlaveRegs[MBSL_R_FLOW];
      logMessage(LOG_INFO, "Получены настройки потока");
    }

    if (SlaveRegs[MBSL_R_DELTA] != Data.Delta && SlaveRegs[MBSL_R_DELTA] < 101 && SlaveRegs[MBSL_R_DELTA] > 29)
    {
      TopStringUpdate = true;
      flowChanged = true;
      Data.Delta = (byte)SlaveRegs[MBSL_R_DELTA];
      logMessage(LOG_INFO, "Получены настройки дельты");
    }

    if (SlaveRegs[MBSL_R_OC_VALVE] != ValveOpen && SlaveRegs[MBSL_R_OC_VALVE] < 5)
    {
      valveChanged = true;
      ValveOpen = (byte)SlaveRegs[MBSL_R_OC_VALVE];
      switch (ValveOpen)
      {
      case 1:
        ValveSetH();
        break;
      case 2:
        ValveSetL();
        break;
      case 4:
        ValveSetN2();
        break;
      default:
        ValveSetOff();
        break;
      }
      if (!ValveOpen)
      {
        ValveSetOff();
      }
      logMessage(LOG_INFO, "Изменено состояние клапанов: " + String(ValveOpen));
    }

    if ((SlaveRegs[MBSL_R_CHANNELS] >> 8) != Data.ChannelsEnable && (SlaveRegs[MBSL_R_CHANNELS] >> 8) < 16)
    {
      channelsChanged = true;
      byte newD = (byte)(SlaveRegs[MBSL_R_CHANNELS] >> 8);
      byte xorD = Data.ChannelsEnable ^ newD;

      for (size_t i = 0; i < 4; i++)
      {
        if (!bitRead(xorD, i))
          continue;

        if (bitRead(newD, i))
        {
          *Channels[i] = true;
          Channels[i]->FlowStabilized = false;
          Channels[i]->setAS200SetFlow(Data.Flow);
          Channels[i]->setIsFlowSet(false);
        }
        else
        {
          *Channels[i] = false;
          Channels[i]->setAS200SetFlow(0);
          Channels[i]->setIsFlowSet(false);
          Channels[i]->FlowStabilized = false;
        }
      }
      logMessage(LOG_INFO, "Изменены настройки каналов");
    }

    if (mbChanged)
    {
      SlaveSerial.end();
      SlaveSerial.begin(4800 * Data.ModBusSpeed, SERIAL_8N2);
      MbSlave.begin(Data.ModBusAdr, 4800 * Data.ModBusSpeed, SERIAL_8N2);
      MbSlave.poll();
      logMessage(LOG_MODBUS, "Настройки ModBus изменены");
    }
    if (flowChanged || (ValveOpen && valveChanged))
    {
      for (size_t i = 0; i < 4; i++)
      {
        if (Channels[i])
          Channels[i]->FlowStabilized = false;
      }
    }
    if (flowChanged || channelsChanged || mbChanged)
      safeEEPROMWrite();
  }
}