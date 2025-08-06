#include "logs.h"

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
