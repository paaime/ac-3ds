#ifndef AC3DS_ACRE_SELECT_H
#define AC3DS_ACRE_SELECT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool active;
    int col, row;
    int player_col, player_row; /* where the player was at the tap */
    uint64_t since_ms;
} AcreSelect;

AcreSelect acre_select_none(void);
AcreSelect acre_select_tap(AcreSelect s, int col, int row, int player_col, int player_row, uint64_t now_ms);
AcreSelect acre_select_step(AcreSelect s, int player_col, int player_row, uint64_t now_ms, uint64_t timeout_ms);
void acre_select_shown(AcreSelect s, int player_col, int player_row, int* col, int* row);

/* Town acres are blocks 1..cols, 1..rows of `block_world` units each; false and -1 off them. */
bool acre_of_world(float x, float z, float block_world, int cols, int rows, int* col, int* row);

#endif /* AC3DS_ACRE_SELECT_H */
