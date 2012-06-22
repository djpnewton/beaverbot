#include "kowhai_log.h"

#include <stdio.h>
#include <stdarg.h>

void kowhai_log(char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}
