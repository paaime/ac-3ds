#ifndef AC3DS_PLAY_3DS_H
#define AC3DS_PLAY_3DS_H

/* The game's play scene (the town, its buildings, the title's demo) when it is
 * what runs; NULL in the other scenes (player select, NES games). */

#include "game.h"
#include "m_play.h"

static inline GAME_PLAY* play3ds_current(void)
{
    return gamePT != NULL && gamePT->exec == play_main ? (GAME_PLAY*)gamePT : NULL;
}

#endif /* AC3DS_PLAY_3DS_H */
