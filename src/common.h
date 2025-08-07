#pragma once
#ifndef ValvesControl_Common_H
#define ValvesControl_Common_H

#include "main.h"
#include <EEPROM.h>

extern unsigned long progressBarTime;
#define TIME_TO_WARNING_H   4
#define TIME_TO_WARNING_L   4
#define TIME_TO_WARNING_N2  6
#define TIME_TO_STOP        2
#define BLINK_PERIOD        250

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
void displayProgressBar();
void clearProgressBar();

#endif