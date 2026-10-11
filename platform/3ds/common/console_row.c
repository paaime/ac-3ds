#include "console_row.h"

#include <stdarg.h>
#include <stdio.h>

#define MAX_ROW_WIDTH CONSOLE_ROW_WIDTH_TOP

static void console_row_move(int row)
{
    printf("\x1b[%d;1H", row);
}

void console_row_print(int row, int width, const char* format, ...)
{
    char text[MAX_ROW_WIDTH + 1];
    int clamped = width < 0 ? 0 : (width > MAX_ROW_WIDTH ? MAX_ROW_WIDTH : width);
    va_list args;

    va_start(args, format);
    vsnprintf(text, (size_t)clamped + 1, format, args);
    va_end(args);

    console_row_move(row);
    printf("%-*s", clamped, text);
}
