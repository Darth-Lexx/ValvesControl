#include "common.h"
#include "logs.h"

unsigned long progressBarTime = 0;
unsigned long blinkTime = 0;
float hTime = (60.0f * TIME_TO_WARNING_H * 1000) / SCREEN_WIDTH;
float lTime = (60.0f * TIME_TO_WARNING_L * 1000) / SCREEN_WIDTH;
float n2Time = (60.0f * TIME_TO_WARNING_N2 * 1000) / SCREEN_WIDTH;
bool stop = false;
bool stopOn = false;

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

void safeEEPROMWrite()
{
    uint8_t bytesWritten = 0;
    EEPROMData currentData;

    // 1. Читаем текущие данные
    EEPROM.get(0, currentData);

    // 2. Сравниваем и записываем побайтово
    const uint8_t *newData = (uint8_t *)&Data;
    const uint8_t *oldData = (uint8_t *)&currentData;

    for (size_t i = 0; i < sizeof(EEPROMData); i++)
    {
        if (newData[i] != oldData[i])
        {
            // 3. Записываем только изменённые байты
            EEPROM.update(i, newData[i]);
            bytesWritten++;
        }
    }

    if (bytesWritten > 0)
    {
        char buf[43];
        snprintf(buf, sizeof(buf), "EEPROM: изменено %u из %u байт", bytesWritten, sizeof(EEPROMData));
        logMessage(LOG_INFO, buf);
    }
}

void pinInit()
{
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
    pinMode(PIN_REL_VALVE, OUTPUT);

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
}

void startSettings()
{
    // проверка пина сброса настроек
    bool reset = !digitalRead(PIN_RESET);

    if (reset)
        logMessage(LOG_WARNING, "Reset pin is active, settings will be reset");

    // чтение данных из EEPROM
    EEPROM.get(0, Data);

    display.print(".");
    display.display();
    logMessage(LOG_INFO, "EEPROM data read");

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
}

void UARTinit()
{
    // инициализация порта и модбас связи с ПК
    SlaveSerial.begin(Data.ModBusSpeed * 4800, SERIAL_8N2);
    MbSlave.begin(Data.ModBusAdr, Data.ModBusSpeed * 4800, SERIAL_8N2);

    display.print(".");
    display.display();
    logMessage(LOG_MODBUS, "ModBus Slave initialized. Address: " + String(Data.ModBusAdr) + ", Speed: " + String(Data.ModBusSpeed * 4800) + " baud");

    // Инициализация связи с AS200 и AFM07
    MasterSerialAS200.begin(38400, SERIAL_8N2);
    AS200Master.begin(38400, SERIAL_8N2);
    MasterSerialAFM07.begin(115200, SERIAL_8N2);
    AFM07Master.begin(115200, SERIAL_8N2);

    display.print(".");
    display.display();
    logMessage(LOG_MODBUS, "AS200 and AFM07 communication ports initialized");
}

void AS200AFM07Init()
{
    for (byte i = 0; i < 4; i++)
    {
        Channels[i]->AS200MbAdr = 1 + i * 2;
        Channels[i]->AFM07MbAdr = 2 + i * 2;
        Channels[i]->setAS200SetFlow(Data.Flow);

        logMessage(LOG_MODBUS, "Channel " + String(i + 1) + " - AS200 address: " + String(Channels[i]->AS200MbAdr) + ", AFM07 address: " + String(Channels[i]->AFM07MbAdr) + ", Set flow: " + String(Data.Flow));
        // Чтение регистров
        Channels[i]->AS200MbError = AS200Master.readHoldingRegisters(Channels[i]->AS200MbAdr, AS200_FLOW_H, Channels[i]->AS200SetFlowArray, array_count(Channels[i]->AS200SetFlowArray));
        Channels[i]->AFM07MbError = AFM07Master.readHoldingRegisters(Channels[i]->AFM07MbAdr, 0, Channels[i]->AFM07Reg, array_count(Channels[i]->AFM07Reg));

        // две попытки повторного чтения при ошибке
        if (Channels[i]->AS200MbError)
        {
            Channels[i]->AS200MbError = AS200Master.readHoldingRegisters(Channels[i]->AS200MbAdr, AS200_FLOW_H, Channels[i]->AS200SetFlowArray, array_count(Channels[i]->AS200SetFlowArray));
            if (Channels[i]->AS200MbError)
                Channels[i]->AS200MbError = AS200Master.readHoldingRegisters(Channels[i]->AS200MbAdr, AS200_FLOW_H, Channels[i]->AS200SetFlowArray, array_count(Channels[i]->AS200SetFlowArray));
        }
        if (Channels[i]->AFM07MbError)
        {
            Channels[i]->AFM07MbError = AFM07Master.readHoldingRegisters(Channels[i]->AFM07MbAdr, 0, Channels[i]->AFM07Reg, array_count(Channels[i]->AFM07Reg));
            if (Channels[i]->AFM07MbError)
                Channels[i]->AFM07MbError = AFM07Master.readHoldingRegisters(Channels[i]->AFM07MbAdr, 0, Channels[i]->AFM07Reg, array_count(Channels[i]->AFM07Reg));
        }

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

            logMessage(LOG_ERROR, "Channel " + String(i + 1) + " activation failed. AS200 error: " + String(Channels[i]->AS200MbError) + ", AFM07 error: " + String(Channels[i]->AFM07MbError) + ", AFM07 status: " + String(Channels[i]->AFM07Reg[AFM07_STATUS]));
        }

        display.print(".");
        display.display();
    }
}

void displayDrawNormal()
{
    display.clearDisplay();
    display.setCursor(1, 0);
    char buffer[23];
    snprintf(buffer, sizeof(buffer), "%-3d %-6ld   %-4d %-3d", Data.ModBusAdr, (long)Data.ModBusSpeed * 4800, Data.Flow, Data.Delta);
    display.print(buffer);
    display.drawLine(0, 8, 128, 8, WHITE);
    display.drawLine(0, 56, 128, 56, WHITE);
    display.drawLine(0, 31, 128, 31, WHITE);
    display.drawLine(62, 8, 62, 56, WHITE);
    display.setCursor(CH1_TL);
    display.print("Channel 1");
    display.setCursor(CH2_TL);
    display.print("Channel 2");
    display.setCursor(CH3_TL);
    display.print("Channel 3");
    display.setCursor(CH4_TL);
    display.print("Channel 4");

    display.setCursor(CH1_STR2);
    display.print("0");
    display.setCursor(CH2_STR2);
    display.print("0");
    display.setCursor(CH3_STR2);
    display.print("0");
    display.setCursor(CH4_STR2);
    display.print("0");
    display.display();
}

void displayProgressBar()
{
    if (!ValveOpen)
        return;
    unsigned long t = millis() - progressBarTime;
    float k;

    if (ValveOpen == 1)
        k = t / hTime;
    if (ValveOpen == 2)
        k = t / lTime;
    if (ValveOpen == 4)
        k = t / n2Time;

    if ((byte)k > 128)
    {
        blinkTime = t;
        stop = true;
    }

    if (stop)
    {
        if (t > TIME_TO_STOP * 60.0f * 1000)
        {
            display.fillRect(BOTTOM_RECR_TL, 128, 7, BLACK);
            display.display();
            stop = false;
            stopOn = false;
            ValveSetOff();
            return;
        }

        if (t - blinkTime < BLINK_PERIOD)
            return;

        blinkTime = t;
        display.fillRect(BOTTOM_RECR_TL, 128, 7, stopOn ? BLACK : WHITE);
        stopOn = !stopOn;
        display.display();
        return;
    }

    display.fillRect(BOTTOM_RECR_TL, (byte)k, 7, WHITE);
    display.display();
}

void clearProgressBar()
{
    display.fillRect(BOTTOM_RECR_TL, 128, 7, BLACK);
    display.display();
    stop = false;
    stopOn = false;
}
