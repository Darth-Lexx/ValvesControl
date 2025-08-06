#include "main.h"
#include "logs.h"
#include "common.h"

// Инициализация времени старта системы
uint32_t systemOverflows = 0;
unsigned long systemStartTime = 0;
unsigned long lastMillisCheck = 0;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

ModbusRTUSlave MbSlave(SlaveSerial);
ModbusRTUMaster AS200Master(MasterSerialAS200, PIN_RE_AS200, PIN_DE_AS200);
ModbusRTUMaster AFM07Master(MasterSerialAFM07, PIN_DE_AFM07);

uint16_t SlaveRegs[17] = {0};
uint16_t SlaveRWRegsActual[2] = {0};

// Флаг подачи газа
byte ValveOpen = 0;
// Флаг готовности каналов
byte ChannelReady = 0;
// Флаг опроса каналов
byte NextChannel = 0;

ChannelStruct Channel1Data;
ChannelStruct Channel2Data;
ChannelStruct Channel3Data;
ChannelStruct Channel4Data;
ChannelStruct *Channels[4] = {&Channel1Data, &Channel2Data, &Channel3Data, &Channel4Data};

EEPROMData Data;

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

  // открытие клапанов сброса давления
  digitalWrite(PIN_H_REL, PIN_ON);
  digitalWrite(PIN_L_REL, PIN_ON);
  digitalWrite(PIN_N2_REL, PIN_ON);

  display.print(".");
  display.display();
  logMessage(LOG_INFO, "Pressure relief valves opened");

  delay(2000);

  // закрытие клапанов сброса давления
  digitalWrite(PIN_H_REL, PIN_OFF);
  digitalWrite(PIN_L_REL, PIN_OFF);
  digitalWrite(PIN_N2_REL, PIN_OFF);

  display.print(".");
  display.display();
  logMessage(LOG_INFO, "Pressure relief valves closed");

  // включение AS200
  digitalWrite(PIN_CTRL1_POWER, PIN_ON);
  digitalWrite(PIN_CTRL2_POWER, PIN_ON);
  digitalWrite(PIN_CTRL3_POWER, PIN_ON);
  digitalWrite(PIN_CTRL4_POWER, PIN_ON);

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
  if (MbSlave.poll())
  {
    if (SlaveRegs[MBSL_R_ADR] != Data.ModBusAdr)
    {
      // TODO
    }

    if (SlaveRegs[MBSL_R_SPEED] != Data.ModBusSpeed)
    {
      // TODO
    }

    if (SlaveRegs[MBSL_R_FLOW] != Data.Flow)
    {
      // TODO
    }

    if (SlaveRegs[MBSL_R_DELTA] != Data.Delta)
    {
      // TODO
    }

    if ((SlaveRegs[MBSL_R_CHANNELS] >> 8) != Data.ChannelsEnable)
    {
      // TODO
    }
  }
}