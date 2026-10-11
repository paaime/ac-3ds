#include "text_wrap.h"

#include <stddef.h>
#include <string.h>

/* Whole words while they fit, or a too-long word's first `width` characters. */
static int fill_line(const char** p, int width, char* line)
{
    const char* s = *p;
    int len = 0;
    while (*s == ' ') {
        s++;
    }
    while (*s != '\0' && *s != '\n') {
        const char* word = s;
        while (*s != '\0' && *s != ' ' && *s != '\n') {
            s++;
        }
        int word_len = (int)(s - word);
        int needed = (len > 0 ? len + 1 : 0) + word_len;
        if (needed > width && len > 0) {
            s = word;
            break;
        }
        if (needed > width) { /* alone and still too long: cut it */
            memcpy(line, word, (size_t)width);
            len = width;
            s = word + width;
            break;
        }
        if (len > 0) {
            line[len++] = ' ';
        }
        memcpy(line + len, word, (size_t)word_len);
        len += word_len;
        while (*s == ' ') {
            s++;
        }
    }
    line[len] = '\0';
    *p = s;
    return len;
}

int text_wrap(const char* text, int width, char lines[][TEXT_WRAP_MAX_WIDTH + 1], int max_lines)
{
    if (text == NULL || *text == '\0' || width <= 0 || max_lines <= 0) {
        return 0;
    }
    width = width > TEXT_WRAP_MAX_WIDTH ? TEXT_WRAP_MAX_WIDTH : width;
    const char* p = text;
    int n = 0;
    while (n < max_lines) {
        fill_line(&p, width, lines[n++]);
        if (*p == '\n') {
            p++;
        } else if (*p == '\0') {
            break;
        }
    }
    return n;
}

int text_center_column(int length, int width)
{
    return length >= width ? 1 : (width - length) / 2 + 1;
}
