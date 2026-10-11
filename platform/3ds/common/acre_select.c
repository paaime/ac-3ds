#include "acre_select.h"

#define FIRST_TOWN_BLOCK 1 /* block 0 is the border cliff */

AcreSelect acre_select_none(void)
{
    AcreSelect s = { false, -1, -1, -1, -1, 0 };
    return s;
}

AcreSelect acre_select_tap(AcreSelect s, int col, int row, int player_col, int player_row, uint64_t now_ms)
{
    (void)s; /* a tap replaces what was chosen */
    if (col == player_col && row == player_row) {
        return acre_select_none();
    }
    AcreSelect chosen = { true, col, row, player_col, player_row, now_ms };
    return chosen;
}

AcreSelect acre_select_step(AcreSelect s, int player_col, int player_row, uint64_t now_ms, uint64_t timeout_ms)
{
    if (!s.active) {
        return s;
    }
    bool moved = player_col != s.player_col || player_row != s.player_row;
    bool expired = now_ms >= s.since_ms && now_ms - s.since_ms >= timeout_ms;
    return moved || expired ? acre_select_none() : s;
}

void acre_select_shown(AcreSelect s, int player_col, int player_row, int* col, int* row)
{
    *col = s.active ? s.col : player_col;
    *row = s.active ? s.row : player_row;
}

bool acre_of_world(float x, float z, float block_world, int cols, int rows, int* col, int* row)
{
    float across = x / block_world - FIRST_TOWN_BLOCK;
    float down = z / block_world - FIRST_TOWN_BLOCK;
    bool on_map = across >= 0.0f && down >= 0.0f && across < (float)cols && down < (float)rows;
    *col = on_map ? (int)across : -1;
    *row = on_map ? (int)down : -1;
    return on_map;
}
