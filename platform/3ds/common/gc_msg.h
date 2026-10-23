#ifndef AC3DS_GC_MSG_H
#define AC3DS_GC_MSG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GC_MSG_CONTROL     0x7F
#define GC_MSG_TAG         0x80
#define GC_MSG_NEW_LINE    0xCD
#define GC_MSG_ASIDE_GREY  0x96
#define GC_MSG_CHOICES_MAX 6
#define GC_MSG_NAME_CHARS  8

typedef enum {
    GC_MSG_LAST = 0x00,
    GC_MSG_CONTINUE = 0x01,
    GC_MSG_CLEAR = 0x02,
    GC_MSG_CURSOR_SET_TIME = 0x03,
    GC_MSG_BUTTON = 0x04,
    GC_MSG_COLOR = 0x05,
    GC_MSG_DEMO_ORDER_NPC0 = 0x09,
    GC_MSG_SELECT_WINDOW = 0x0D,
    GC_MSG_NEXT_MESSAGE_0 = 0x0F,
    GC_MSG_NEXT_MESSAGE_1 = 0x10,
    GC_MSG_NEXT_MESSAGE_2 = 0x11,
    GC_MSG_NEXT_MESSAGE_3 = 0x12,
    GC_MSG_SELECT_STRING_2 = 0x16,
    GC_MSG_SELECT_STRING_3 = 0x17,
    GC_MSG_SELECT_STRING_4 = 0x18,
    GC_MSG_FORCE_NEXT = 0x19,
    GC_MSG_PLAYER_NAME = 0x1A,
    GC_MSG_SOUND_CUT = 0x51,
    GC_MSG_SELECT_NO_B = 0x5E,
    GC_MSG_NEXT_MESSAGE_4 = 0x77,
    GC_MSG_NEXT_MESSAGE_5 = 0x78,
    GC_MSG_SELECT_STRING_5 = 0x79,
    GC_MSG_SELECT_STRING_6 = 0x7A,
} GcMsgCode;

typedef struct {
    uint8_t* data;
    size_t size;
    size_t len;
    bool ok;
} GcMsg;

typedef struct {
    uint16_t string_id;
    uint16_t next_msg;
} GcMsgChoice;

typedef int (*GcMsgCodeSize)(const uint8_t* code);

GcMsg gc_msg_start(uint8_t* data, size_t size);
GcMsg gc_msg_text(GcMsg m, const char* markup);
GcMsg gc_msg_menu(GcMsg m, const GcMsgChoice* choices, int count);
GcMsg gc_msg_continue(GcMsg m);
GcMsg gc_msg_last(GcMsg m);

size_t gc_msg_insert_choice(const uint8_t* in, size_t len, uint8_t* out, size_t size, GcMsgChoice choice,
                            GcMsgCodeSize code_size);

bool gc_msg_fits(const char* markup, int max_chars, int max_lines);

#endif /* AC3DS_GC_MSG_H */
