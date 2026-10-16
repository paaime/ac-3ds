#ifndef AC3DS_CONSOLE_ROW_H
#define AC3DS_CONSOLE_ROW_H

/* Columns minus one (8x8 font), so the cursor never wraps onto the next row. */
#define CONSOLE_ROW_WIDTH_TOP    49
#define CONSOLE_ROW_WIDTH_BOTTOM 39

/* `row` is 1-based. */
void console_row_print(int row, int width, const char* format, ...)
    __attribute__((format(printf, 3, 4)));

#endif /* AC3DS_CONSOLE_ROW_H */
