#ifndef AC3DS_TOWN_MAP_3DS_H
#define AC3DS_TOWN_MAP_3DS_H

/* The game's own town map on the bottom screen (town_map_3ds.c). Plain C
 * types: bottom_3ds.c doesn't include the game's headers. */

#include <stdbool.h>

#include "bottom_layout.h"

#define TOWN_MAP_BLOCK_WORLD 640.0f /* world units per acre (mFI_BK_WORLDSIZE_X_F) */
#define TOWN_MAP_COLS        5      /* FG_BLOCK_X_NUM */
#define TOWN_MAP_ROWS        6      /* FG_BLOCK_Z_NUM */

typedef struct {
    bool available; /* the play scene, not the title demo */
    bool player_known;
    float player_x, player_z; /* world position (outdoors, or the door indoors) */
    bool indoors;
} TownMapState;

TownMapState town_map_read(void);

/* Inside a bottom-screen list; rects in screen pixels, cursor on acre (col, row)
 * or the player's for -1. False where the game wouldn't open its map. */
bool town_map_draw(PxRect grid, PxRect panel, int col, int row);

/* False off the tiles or where the map doesn't show. */
bool town_map_acre_at(PxRect grid, int x, int y, int* col, int* row);

void town_map_cursor_sound(void);

#endif /* AC3DS_TOWN_MAP_3DS_H */
