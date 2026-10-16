#include <3ds.h>
#include <stdlib.h>

#include "sdl_compat/SDL.h"

#define NS_PER_MS 1000000LL

struct SDL_mutex {
    LightLock lock;
};

SDL_mutex* SDL_CreateMutex(void)
{
    SDL_mutex* mutex = malloc(sizeof(*mutex));
    if (mutex != NULL) {
        LightLock_Init(&mutex->lock);
    }
    return mutex;
}

void SDL_DestroyMutex(SDL_mutex* mutex)
{
    free(mutex);
}

int SDL_LockMutex(SDL_mutex* mutex)
{
    if (mutex == NULL) {
        return -1;
    }
    LightLock_Lock(&mutex->lock);
    return 0;
}

int SDL_UnlockMutex(SDL_mutex* mutex)
{
    if (mutex == NULL) {
        return -1;
    }
    LightLock_Unlock(&mutex->lock);
    return 0;
}

void SDL_Delay(uint32_t ms)
{
    svcSleepThread((s64)ms * NS_PER_MS);
}
