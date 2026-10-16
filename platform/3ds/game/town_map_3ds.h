#ifndef AC3DS_TOWN_MAP_3DS_H
#define AC3DS_TOWN_MAP_3DS_H

#include <stdbool.h>

#include "bottom_layout.h"

#define TOWN_MAP_BLOCK_WORLD 640.0f /* world units per acre (mFI_BK_WORLDSIZE_X_F) */
#define TOWN_MAP_COLS        5      /* FG_BLOCK_X_NUM */
#define TOWN_MAP_ROWS        6      /* FG_BLOCK_Z_NUM */

typedef struct {
    bool available;
    bool player_known;
    float player_x, player_z; /* world position; indoors, the door's */
    bool indoors;
} TownMapState;

TownMapState town_map_read(void);

/* In a bottom-screen list; cursor on (col, row), the player's acre for -1; false where the game has no map. */
bool town_map_draw(PxRect grid, PxRect panel, int col, int row);

bool town_map_acre_at(PxRect grid, int x, int y, int* col, int* row);

void town_map_cursor_sound(void);

#endif /* AC3DS_TOWN_MAP_3DS_H */
