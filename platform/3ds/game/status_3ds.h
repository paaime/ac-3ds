#ifndef AC3DS_STATUS_3DS_H
#define AC3DS_STATUS_3DS_H

#include <stdbool.h>

void status3ds_init(void);
/* false: keep the console untouched (stdout goes to the log in verbose mode). */
void status3ds_console(bool on);

void status3ds_step(const char* format, ...) __attribute__((format(printf, 1, 2)));
void status3ds_loading(const char* what);
/* '\n' in message starts a paragraph; waits for START. */
void status3ds_fatal(const char* message);
/* Waits for A; false if the system asked the game to close meanwhile. */
bool status3ds_warning(const char* message);
/* True for A (try anyway), false for START or if the system asked the game to close. */
bool status3ds_ask(const char* heading, const char* message);

void status3ds_set_visible(bool visible);

bool status3ds_console_available(void);

/* After the frame is submitted. */
void status3ds_frame(void);

void status3ds_log_summary(void);

#endif /* AC3DS_STATUS_3DS_H */
