#ifndef AC3DS_TEXDUMP_3DS_H
#define AC3DS_TEXDUMP_3DS_H

/* The "Dump textures" setting (texdump_3ds.c): each texture the renderer uses,
 * once, as a TGA in the data folder's dump/. The files are the game's own art,
 * for the user's eyes only. */

#include <stdbool.h>

void texdump3ds_enable(bool on);

#endif /* AC3DS_TEXDUMP_3DS_H */
