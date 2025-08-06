#pragma once
#ifndef ValvesControl_Logs_H
#define ValvesControl_Logs_H

#include <Arduino.h>

#define ANSI_COLORS  // Раскомментировать для включения цветного вывода

// В секцию enum LogLevel добавляем:
enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR, 
    LOG_DEBUG,
    LOG_MODBUS  // Новый тип для ModBus-сообщений
};


#ifdef ANSI_COLORS
// ANSI Color Codes
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#else
// Пустые define для отключения цветов
#define ANSI_RESET   ""
#define ANSI_RED     ""
#define ANSI_GREEN   ""
#define ANSI_YELLOW  ""
#define ANSI_BLUE    ""
#define ANSI_MAGENTA ""
#define ANSI_CYAN    ""
#define ANSI_WHITE   ""
#endif

// Глобальная переменная для времени старта системы
extern uint32_t systemOverflows;
extern unsigned long systemStartTime;
extern unsigned long lastMillisCheck;

void logMessage(LogLevel level, const String &message);
String formatTime();
void updateTimeTracker();

#endif