#ifndef AC3DS_TEXT_WRAP_H
#define AC3DS_TEXT_WRAP_H

#define TEXT_WRAP_MAX_WIDTH 49 /* the top screen's console row */
#define TEXT_WRAP_MAX_LINES 24

int text_wrap(const char* text, int width, char lines[][TEXT_WRAP_MAX_WIDTH + 1], int max_lines);

/* 1-based. */
int text_center_column(int length, int width);

#endif /* AC3DS_TEXT_WRAP_H */
