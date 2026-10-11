#ifndef AC3DS_STATUS_3DS_H
#define AC3DS_STATUS_3DS_H

/* The bottom screen's console: loading screen or boot steps, start-up errors,
 * and the developer build's live stats (memory peaks). */

#include <stdbool.h>

void status3ds_init(void);
/* false: keep the console untouched (stdout goes to the log in verbose mode). */
void status3ds_console(bool on);

/* Boot log line, in the log file; on screen in the developer build only. */
void status3ds_step(const char* format, ...) __attribute__((format(printf, 1, 2)));
/* The player build's loading screen: what is happening (a short line). */
void status3ds_loading(const char* what);
/* Boot failure: shows `message` ('\n' starts a paragraph), waits for START. */
void status3ds_fatal(const char* message);
/* Before the game: the message, and waits for A (the game goes on); false
 * if the system asked the game to close meanwhile. */
bool status3ds_warning(const char* message);
/* Before the game: the message under `heading`; true for A (try anyway), false
 * for START or if the system asked the game to close. */
bool status3ds_ask(const char* heading, const char* message);

/* false while the bottom screen's own view owns it. */
void status3ds_set_visible(bool visible);

/* false with the detailed log on: stdout goes to the log, the console is off. */
bool status3ds_console_available(void);

/* Once per frame, after the frame is submitted. */
void status3ds_frame(void);

/* Peak memory use so far, for the log at exit. */
void status3ds_log_summary(void);

#endif /* AC3DS_STATUS_3DS_H */
