/* heap_3ds.c - memory layout (replaces libctru's weak __system_allocateHeaps). A regular heap at
 * 0x08000000 would collide with N64 segments 8-D in emu64's seg2k0, so all free memory is linear
 * (above 0x30000000):  [ malloc heap: main stack, then newlib malloc ][ linearAlloc pool ] */
#include <3ds.h>

#include "heap_3ds.h"
#include "mem_layout.h"

#define MB                (1024u * 1024u)
#define LINEAR_POOL_BYTES (24u * MB) /* textures, vertex ring, command buffer */
#define MIN_POOL_BYTES    (8u * MB)  /* framebuffers, command buffer, vertex ring + some textures */
#define MIN_MALLOC_BYTES  (72u * MB) /* town's measured peak 63.7 MB + margin */
#define TIGHT_MALLOC_BYTES (64u * MB) /* the peak, no margin: an original 3DS's 75 MB (ac3ds_main.c decides) */
#define SHRINK_STEP_BYTES (4u * MB)
#define RESCUE_MALLOC_BYTES (4u * MB) /* main stack (1 MB), console, the error screen */
#define RESCUE_POOL_BYTES   (2u * MB) /* gfxInitDefault's framebuffers */

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap;
extern u32 __ctru_heap_size;
extern u32 __ctru_linear_heap;
extern u32 __ctru_linear_heap_size;

static Heap3dsReport s_report;

static u32 free_application_memory(void)
{
    Handle reslimit = 0;
    if (R_FAILED(svcGetResourceLimit(&reslimit, CUR_PROCESS_HANDLE))) {
        svcBreak(USERBREAK_PANIC);
    }
    s64 max_commit = 0;
    s64 current_commit = 0;
    ResourceLimitType type = RESLIMIT_COMMIT;
    svcGetResourceLimitLimitValues(&max_commit, reslimit, &type, 1);
    svcGetResourceLimitCurrentValues(&current_commit, reslimit, &type, 1);
    svcCloseHandle(reslimit);
    return (u32)(max_commit - current_commit) & ~(MEM_LAYOUT_PAGE - 1u);
}

static u32 commit_linear(u32 bytes)
{
    u32 addr = 0;
    Result rc = svcControlMemory(&addr, 0, 0, bytes, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);
    return R_SUCCEEDED(rc) ? addr : 0;
}

void __system_allocateHeaps(void)
{
    u32 available = free_application_memory();
    s_report.available_bytes = available;

    s_report.needed_bytes = MIN_MALLOC_BYTES + MIN_POOL_BYTES;
    s_report.tight_needed_bytes = TIGHT_MALLOC_BYTES + MIN_POOL_BYTES;
    MemLayout layout =
        mem_layout_plan_or_tight(available, LINEAR_POOL_BYTES, MIN_POOL_BYTES, MIN_MALLOC_BYTES, TIGHT_MALLOC_BYTES);
    u32 base = 0;
    while (layout.ok && (base = commit_linear(layout.total_bytes)) == 0) {
        s_report.shrink_steps++;
        u32 smaller = layout.total_bytes > SHRINK_STEP_BYTES ? layout.total_bytes - SHRINK_STEP_BYTES : 0;
        layout = mem_layout_plan_or_tight(smaller, LINEAR_POOL_BYTES, MIN_POOL_BYTES, MIN_MALLOC_BYTES,
                                          TIGHT_MALLOC_BYTES);
    }
    if (!layout.ok || base == 0) {
        s_report.rescue = true;
        u32 rescue = RESCUE_POOL_BYTES + RESCUE_MALLOC_BYTES;
        layout = mem_layout_plan(available < rescue ? available : rescue, RESCUE_POOL_BYTES, RESCUE_POOL_BYTES,
                                 RESCUE_MALLOC_BYTES);
        base = layout.ok ? commit_linear(layout.total_bytes) : 0;
    }
    if (!layout.ok || base == 0) {
        svcBreak(USERBREAK_PANIC); /* not even a few MB: nothing can run */
    }

    /* No regular heap: these name both parts so __libctru_exit frees them. */
    __ctru_heap = base;
    __ctru_heap_size = layout.malloc_bytes;
    __ctru_linear_heap = base + layout.malloc_bytes;
    __ctru_linear_heap_size = layout.pool_bytes;
    mappableInit(OS_MAP_AREA_BEGIN, OS_MAP_AREA_END);

    /* newlib's heap; libctru's initSystem then carves the main stack from its start */
    fake_heap_start = (char*)base;
    fake_heap_end = fake_heap_start + layout.malloc_bytes;

    s_report.layout = layout;
    s_report.base = base;
}

Heap3dsReport heap3ds_report(void)
{
    return s_report;
}
