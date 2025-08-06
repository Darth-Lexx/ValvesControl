#pragma once
#ifndef ValvesControl_Common_H
#define ValvesControl_Common_H

#include "main.h"
#include <EEPROM.h>

void setError(uint16_t (&data)[17], ErrorGroups errorType, uint8_t device_num, uint8_t error);
uint8_t getError(const uint16_t (&data)[17], ErrorGroups errorType, uint8_t device_num);
//инициализация
void safeEEPROMWrite();
void pinInit();
void startSettings();
void UARTinit();
void AS200AFM07Init();
void displayDrawNormal();
//---

#endif