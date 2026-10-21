#ifndef AC3DS_LOOK_TALK_H
#define AC3DS_LOOK_TALK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gc_msg.h"
#include "look.h"
#include "port_msg.h"

#define LOOK_TALK_MSG_FIRST PORT_MSG_LOOK_FIRST
#define LOOK_TALK_MSG_COUNT PORT_MSG_LOOK_COUNT

typedef enum {
    LOOK_TALK_GYROID,
    LOOK_TALK_ROVER,
} LookTalkSpeaker;

typedef enum {
    LOOK_TALK_STRING_CHANGE = PORT_STRING_LOOK_FIRST,
    LOOK_TALK_STRING_FACE,
    LOOK_TALK_STRING_TONE,
    LOOK_TALK_STRING_DONE,
    LOOK_TALK_STRING_NEXT_FACE,
    LOOK_TALK_STRING_PREVIOUS_FACE,
    LOOK_TALK_STRING_KEEP,
    LOOK_TALK_STRING_PUT_BACK,
    LOOK_TALK_STRING_LIGHTER,
    LOOK_TALK_STRING_DARKER,
    LOOK_TALK_STRING_LIGHT_SHADES,
    LOOK_TALK_STRING_MEDIUM_SHADES,
    LOOK_TALK_STRING_DARK_SHADES,
    LOOK_TALK_STRING_TONE_FIRST,
    LOOK_TALK_STRING_END = LOOK_TALK_STRING_TONE_FIRST + LOOK_TONES
} LookTalkString;

typedef struct {
    bool at_limit;
    int tone;
    uint16_t return_msg;
} LookTalkState;

typedef enum {
    LOOK_TALK_NOTHING,
    LOOK_TALK_START,
    LOOK_TALK_FACE,
    LOOK_TALK_FACE_BACK,
    LOOK_TALK_TONE,
    LOOK_TALK_TONE_BACK,
    LOOK_TALK_SET_TONE,
    LOOK_TALK_FINISH,
} LookTalkEffect;

typedef struct {
    LookTalkEffect effect;
    int delta;
    int tone;
} LookTalkStep;

int look_talk_start(LookTalkSpeaker speaker);
LookTalkSpeaker look_talk_speaker(int msg);
bool look_talk_owns(int msg);
const char* look_talk_string(int id);
const char* look_talk_tone_name(int tone);

bool look_talk_markup(int msg, LookTalkState st, char* out, size_t size);
int look_talk_choices(int msg, GcMsgChoice out[GC_MSG_CHOICES_MAX]);
size_t look_talk_build(int msg, LookTalkState st, uint8_t* out, size_t size);
LookTalkStep look_talk_step(int msg);

#endif /* AC3DS_LOOK_TALK_H */
