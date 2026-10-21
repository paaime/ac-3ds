#include "gc_msg.h"

#include <string.h>

#define CODE_LEN          2
#define ID_LEN            2
#define CHOICES_MIN       2
#define ARG_MAX           255
#define COLOUR_DIGITS     6
#define HEX_BASE          16
#define SOUND_OFF         0
#define SOUND_ON          1
#define EXPRESSION_ORDER  0
#define PLAIN_CHARACTERS  " !\"%&'(),-.:<=>?@"

typedef enum {
    TOKEN_BAD,
    TOKEN_PAGE,
    TOKEN_NAME,
    TOKEN_ASIDE_OPEN,
    TOKEN_ASIDE_CLOSE,
    TOKEN_PAUSE,
    TOKEN_EXPRESSION,
    TOKEN_COLOUR,
} TokenKind;

typedef struct {
    TokenKind kind;
    int value;
} Token;

typedef struct {
    GcMsg m;
    bool aside;
} Writer;

static const uint8_t k_select_codes[GC_MSG_CHOICES_MAX + 1] = {
    [2] = GC_MSG_SELECT_STRING_2, [3] = GC_MSG_SELECT_STRING_3, [4] = GC_MSG_SELECT_STRING_4,
    [5] = GC_MSG_SELECT_STRING_5, [6] = GC_MSG_SELECT_STRING_6,
};

static const uint8_t k_next_codes[GC_MSG_CHOICES_MAX] = {
    GC_MSG_NEXT_MESSAGE_0, GC_MSG_NEXT_MESSAGE_1, GC_MSG_NEXT_MESSAGE_2,
    GC_MSG_NEXT_MESSAGE_3, GC_MSG_NEXT_MESSAGE_4, GC_MSG_NEXT_MESSAGE_5,
};

GcMsg gc_msg_start(uint8_t* data, size_t size)
{
    GcMsg m = { data, data != NULL ? size : 0, 0, data != NULL };
    return m;
}

static GcMsg failed(GcMsg m)
{
    GcMsg out = m;
    out.ok = false;
    return out;
}

static GcMsg put(GcMsg m, const uint8_t* bytes, size_t n)
{
    if (!m.ok || m.size - m.len < n) {
        return failed(m);
    }
    GcMsg out = m;
    memcpy(out.data + out.len, bytes, n);
    out.len += n;
    return out;
}

static GcMsg put_byte(GcMsg m, uint8_t b)
{
    return put(m, &b, 1);
}

static GcMsg put_code(GcMsg m, uint8_t code)
{
    const uint8_t bytes[] = { GC_MSG_CONTROL, code };
    return put(m, bytes, sizeof(bytes));
}

static GcMsg put_u16(GcMsg m, uint16_t v)
{
    const uint8_t bytes[] = { (uint8_t)(v >> 8), (uint8_t)v };
    return put(m, bytes, sizeof(bytes));
}

static GcMsg put_grey(GcMsg m)
{
    const uint8_t bytes[] = { GC_MSG_CONTROL, GC_MSG_COLOR, GC_MSG_ASIDE_GREY, GC_MSG_ASIDE_GREY, GC_MSG_ASIDE_GREY };
    return put(m, bytes, sizeof(bytes));
}

static GcMsg put_expression(GcMsg m, uint8_t expression)
{
    const uint8_t bytes[] = { GC_MSG_CONTROL, GC_MSG_DEMO_ORDER_NPC0, EXPRESSION_ORDER, 0, expression };
    return put(m, bytes, sizeof(bytes));
}

static GcMsg put_colour(GcMsg m, int rgb)
{
    const uint8_t bytes[] = { GC_MSG_CONTROL, GC_MSG_COLOR, (uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb };
    return put(m, bytes, sizeof(bytes));
}

static GcMsg put_sound(GcMsg m, uint8_t on)
{
    const uint8_t bytes[] = { GC_MSG_CONTROL, GC_MSG_SOUND_CUT, on };
    return put(m, bytes, sizeof(bytes));
}

static int number(const char* s, size_t n)
{
    int value = 0;
    for (size_t i = 0; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return -1;
        }
        value = value * 10 + (s[i] - '0');
        if (value > ARG_MAX) {
            return -1;
        }
    }
    return value;
}

static int hex_colour(const char* s, size_t n)
{
    int value = 0;
    if (n != COLOUR_DIGITS) {
        return -1;
    }
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        int digit = c >= '0' && c <= '9' ? c - '0'
                    : c >= 'a' && c <= 'f' ? c - 'a' + 10
                    : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                           : -1;
        if (digit < 0) {
            return -1;
        }
        value = value * HEX_BASE + digit;
    }
    return value;
}

static Token parse_token(const char* s, size_t n)
{
    Token t = { TOKEN_BAD, 0 };
    if (n == 1 && s[0] == 'p') {
        t.kind = TOKEN_PAGE;
    } else if (n == 4 && memcmp(s, "name", 4) == 0) {
        t.kind = TOKEN_NAME;
    } else if (n == 1 && s[0] == '(') {
        t.kind = TOKEN_ASIDE_OPEN;
    } else if (n == 1 && s[0] == ')') {
        t.kind = TOKEN_ASIDE_CLOSE;
    } else if (n > 1 && s[0] == 'c') {
        t.value = hex_colour(s + 1, n - 1);
        t.kind = t.value < 0 ? TOKEN_BAD : TOKEN_COLOUR;
    } else if (n > 1 && (s[0] == 'w' || s[0] == 'e')) {
        t.value = number(s + 1, n - 1);
        t.kind = t.value < 0 ? TOKEN_BAD : s[0] == 'w' ? TOKEN_PAUSE : TOKEN_EXPRESSION;
    }
    return t;
}

static Writer write_token(Writer w, Token t)
{
    Writer out = w;
    switch (t.kind) {
    case TOKEN_PAGE:
        out.m = put_code(put_byte(put_code(w.m, GC_MSG_BUTTON), GC_MSG_NEW_LINE), GC_MSG_CLEAR);
        break;
    case TOKEN_NAME:
        out.m = put_code(w.m, GC_MSG_PLAYER_NAME);
        break;
    case TOKEN_ASIDE_OPEN:
        out.m = w.aside ? failed(w.m) : put_byte(put_sound(put_grey(w.m), SOUND_OFF), '(');
        out.aside = true;
        break;
    case TOKEN_ASIDE_CLOSE:
        out.m = w.aside ? put_sound(put_byte(w.m, ')'), SOUND_ON) : failed(w.m);
        out.aside = false;
        break;
    case TOKEN_PAUSE:
        out.m = put_byte(put_code(w.m, GC_MSG_CURSOR_SET_TIME), (uint8_t)t.value);
        break;
    case TOKEN_EXPRESSION:
        out.m = put_expression(w.m, (uint8_t)t.value);
        break;
    case TOKEN_COLOUR:
        out.m = put_colour(w.m, t.value);
        break;
    default:
        out.m = failed(w.m);
        break;
    }
    return out;
}

static bool plain(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           (c != '\0' && strchr(PLAIN_CHARACTERS, c) != NULL);
}

static Writer write_character(Writer w, char c)
{
    Writer out = w;
    if (c == '\n') {
        out.m = put_byte(w.m, GC_MSG_NEW_LINE);
        out.m = w.aside ? put_grey(out.m) : out.m;
    } else {
        out.m = plain(c) ? put_byte(w.m, (uint8_t)c) : failed(w.m);
    }
    return out;
}

GcMsg gc_msg_text(GcMsg m, const char* markup)
{
    Writer w = { m, false };
    const char* p = markup;
    while (*p != '\0' && w.m.ok) {
        if (*p == '{') {
            const char* end = strchr(p, '}');
            if (end == NULL) {
                return failed(w.m);
            }
            w = write_token(w, parse_token(p + 1, (size_t)(end - p - 1)));
            p = end + 1;
        } else {
            w = write_character(w, *p);
            p++;
        }
    }
    return w.aside ? failed(w.m) : w.m;
}

GcMsg gc_msg_menu(GcMsg m, const GcMsgChoice* choices, int count)
{
    if (count < CHOICES_MIN || count > GC_MSG_CHOICES_MAX) {
        return failed(m);
    }
    GcMsg out = put_code(m, k_select_codes[count]);
    for (int i = 0; i < count; i++) {
        out = put_u16(out, choices[i].string_id);
    }
    out = put_code(put_code(put_code(out, GC_MSG_SELECT_NO_B), GC_MSG_BUTTON), GC_MSG_SELECT_WINDOW);
    for (int i = 0; i < count; i++) {
        out = put_u16(put_code(out, k_next_codes[i]), choices[i].next_msg);
    }
    return gc_msg_continue(put_code(out, GC_MSG_FORCE_NEXT));
}

GcMsg gc_msg_continue(GcMsg m)
{
    return put_code(put_byte(m, GC_MSG_NEW_LINE), GC_MSG_CONTINUE);
}

GcMsg gc_msg_last(GcMsg m)
{
    return put_code(put_byte(m, GC_MSG_NEW_LINE), GC_MSG_LAST);
}

static size_t code_len(const uint8_t* in, size_t at, size_t len, GcMsgCodeSize code_size)
{
    if (in[at] != GC_MSG_CONTROL && in[at] != GC_MSG_TAG) {
        return 1;
    }
    if (at + 1 >= len) {
        return 0;
    }
    int n = code_size(in + at);
    return n >= CODE_LEN && at + (size_t)n <= len ? (size_t)n : 0;
}

static int select_count(const uint8_t* code)
{
    for (int n = CHOICES_MIN; n <= GC_MSG_CHOICES_MAX; n++) {
        if (code[0] == GC_MSG_CONTROL && code[1] == k_select_codes[n]) {
            return n;
        }
    }
    return 0;
}

static int next_index(const uint8_t* code)
{
    for (int i = 0; i < GC_MSG_CHOICES_MAX; i++) {
        if (code[0] == GC_MSG_CONTROL && code[1] == k_next_codes[i]) {
            return i;
        }
    }
    return -1;
}

typedef struct {
    int count;
    bool has_last_next;
} MenuShape;

static MenuShape menu_shape(const uint8_t* in, size_t len, GcMsgCodeSize code_size)
{
    MenuShape shape = { 0, false };
    for (size_t i = 0; i < len;) {
        size_t n = code_len(in, i, len, code_size);
        if (n == 0) {
            MenuShape broken = { 0, false };
            return broken;
        }
        if (shape.count == 0) {
            shape.count = select_count(in + i);
        } else if (next_index(in + i) == shape.count - 1) {
            shape.has_last_next = true;
        }
        i += n;
    }
    return shape;
}

static GcMsg put_select(GcMsg m, const uint8_t* code, int count, uint16_t id)
{
    size_t kept = ID_LEN * (size_t)(count - 1);
    GcMsg out = put(put_code(m, k_select_codes[count + 1]), code + CODE_LEN, kept);
    return put(put_u16(out, id), code + CODE_LEN + kept, ID_LEN);
}

static GcMsg put_nexts(GcMsg m, const uint8_t* code, int count, uint16_t next)
{
    GcMsg out = put_u16(put_code(m, k_next_codes[count - 1]), next);
    return put(put_code(out, k_next_codes[count]), code + CODE_LEN, ID_LEN);
}

size_t gc_msg_insert_choice(const uint8_t* in, size_t len, uint8_t* out, size_t size, GcMsgChoice choice,
                            GcMsgCodeSize code_size)
{
    MenuShape shape = menu_shape(in, len, code_size);
    if (shape.count < CHOICES_MIN || shape.count >= GC_MSG_CHOICES_MAX || !shape.has_last_next) {
        return 0;
    }
    GcMsg m = gc_msg_start(out, size);
    bool selected = false;
    for (size_t i = 0; i < len; i += code_len(in, i, len, code_size)) {
        if (!selected && select_count(in + i) == shape.count) {
            m = put_select(m, in + i, shape.count, choice.string_id);
            selected = true;
        } else if (selected && next_index(in + i) == shape.count - 1) {
            m = put_nexts(m, in + i, shape.count, choice.next_msg);
        } else {
            m = put(m, in + i, code_len(in, i, len, code_size));
        }
    }
    return m.ok ? m.len : 0;
}

typedef struct {
    int chars;
    int lines;
    bool ok;
} Layout;

static Layout layout_add(Layout l, int chars, int max_chars)
{
    Layout out = l;
    out.chars += chars;
    out.ok = l.ok && out.chars <= max_chars;
    return out;
}

static Layout layout_break(Layout l, bool page, int max_lines)
{
    Layout out = { 0, page ? 1 : l.lines + 1, l.ok };
    out.ok = out.ok && out.lines <= max_lines;
    return out;
}

static Layout layout_token(Layout l, Token t, int max_chars, int max_lines)
{
    switch (t.kind) {
    case TOKEN_PAGE:
        return layout_break(l, true, max_lines);
    case TOKEN_NAME:
        return layout_add(l, GC_MSG_NAME_CHARS, max_chars);
    case TOKEN_ASIDE_OPEN:
    case TOKEN_ASIDE_CLOSE:
        return layout_add(l, 1, max_chars);
    case TOKEN_BAD: {
        Layout bad = { 0, 0, false };
        return bad;
    }
    default:
        return l;
    }
}

bool gc_msg_fits(const char* markup, int max_chars, int max_lines)
{
    Layout l = { 0, 1, max_lines >= 1 };
    const char* p = markup;
    while (*p != '\0' && l.ok) {
        if (*p == '{') {
            const char* end = strchr(p, '}');
            if (end == NULL) {
                return false;
            }
            l = layout_token(l, parse_token(p + 1, (size_t)(end - p - 1)), max_chars, max_lines);
            p = end + 1;
        } else {
            l = *p == '\n' ? layout_break(l, false, max_lines) : layout_add(l, 1, max_chars);
            p++;
        }
    }
    return l.ok;
}
