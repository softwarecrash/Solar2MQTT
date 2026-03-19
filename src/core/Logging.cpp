#include "main.h"

#include <stdarg.h>
#include <stdio.h>

#include "core/LogSerial.h"

void writeLog(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    LogSerial.println(buffer);
}
