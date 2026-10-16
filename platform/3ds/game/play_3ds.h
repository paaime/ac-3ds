#ifndef AC3DS_PLAY_3DS_H
#define AC3DS_PLAY_3DS_H

#include "game.h"
#include "m_play.h"

/* The play scene (town, buildings, also the title demo), or NULL in the other scenes. */
static inline GAME_PLAY* play3ds_current(void)
{
    return gamePT != NULL && gamePT->exec == play_main ? (GAME_PLAY*)gamePT : NULL;
}

#endif /* AC3DS_PLAY_3DS_H */
