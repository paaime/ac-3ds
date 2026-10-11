/* SDL.h — the SDL calls jaudio_NES/internal/os.c makes (game/sdl_compat_3ds.c).
 * Only that file has this directory on its include path. */
#ifndef AC3DS_SDL_COMPAT_H
#define AC3DS_SDL_COMPAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_mutex SDL_mutex;

SDL_mutex* SDL_CreateMutex(void);
void SDL_DestroyMutex(SDL_mutex* mutex);
int SDL_LockMutex(SDL_mutex* mutex);
int SDL_UnlockMutex(SDL_mutex* mutex);
void SDL_Delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* AC3DS_SDL_COMPAT_H */
