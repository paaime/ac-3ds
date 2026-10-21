#ifndef AC3DS_ORDINANCE_TALK_H
#define AC3DS_ORDINANCE_TALK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gc_msg.h"
#include "ordinance.h"

#define ORDINANCE_TALK_MSG_FIRST  0x3F91
#define ORDINANCE_TALK_MSG_COUNT  28
#define ORDINANCE_TALK_STRING_LEN 16
#define ORDINANCE_TALK_LINE_CHARS 28
#define ORDINANCE_TALK_PAGE_LINES 4

typedef enum {
    ORDINANCE_TALK_PELLY,
    ORDINANCE_TALK_PHYLLIS,
} OrdinanceTalkSpeaker;

typedef enum {
    ORDINANCE_TALK_STRING_ORDINANCES = 607,
    ORDINANCE_TALK_STRING_EARLY_BIRD,
    ORDINANCE_TALK_STRING_NIGHT_OWL,
    ORDINANCE_TALK_STRING_BELL_BOOM,
    ORDINANCE_TALK_STRING_BEAUTIFUL_TOWN,
    ORDINANCE_TALK_STRING_REPEAL,
    ORDINANCE_TALK_STRING_END
} OrdinanceTalkString;

typedef struct {
    Ordinance current;
    uint32_t wallet;
} OrdinanceTalkState;

typedef enum {
    ORDINANCE_TALK_NOTHING,
    ORDINANCE_TALK_ENACT,
    ORDINANCE_TALK_REPEAL,
} OrdinanceTalkEffect;

typedef struct {
    OrdinanceTalkEffect effect;
    Ordinance ordinance;
    bool back_to_menu;
} OrdinanceTalkStep;

int ordinance_talk_intro(OrdinanceTalkSpeaker speaker);
bool ordinance_talk_owns(int msg);
const char* ordinance_talk_string(int id);

bool ordinance_talk_markup(int msg, OrdinanceTalkState st, char* out, size_t size);
int ordinance_talk_choices(int msg, OrdinanceTalkState st, GcMsgChoice out[GC_MSG_CHOICES_MAX]);
size_t ordinance_talk_build(int msg, OrdinanceTalkState st, uint8_t* out, size_t size);
OrdinanceTalkStep ordinance_talk_step(int msg);

size_t ordinance_talk_sign(Ordinance o, int open_hour, int close_hour, uint8_t* out, size_t size);

#endif /* AC3DS_ORDINANCE_TALK_H */
