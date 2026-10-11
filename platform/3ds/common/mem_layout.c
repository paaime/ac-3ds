#include "mem_layout.h"

static uint32_t page_down(uint32_t v)
{
    return v & ~(MEM_LAYOUT_PAGE - 1u);
}

static uint32_t page_up(uint32_t v)
{
    uint32_t down = page_down(v);
    return down == v ? v : down + MEM_LAYOUT_PAGE;
}

MemLayout mem_layout_plan(uint32_t available, uint32_t pool_bytes, uint32_t min_pool_bytes,
                          uint32_t min_malloc_bytes)
{
    MemLayout l = { false, 0, 0, 0, false };
    uint32_t total = page_down(available);
    uint32_t min_malloc = page_up(min_malloc_bytes);
    uint32_t min_pool = page_up(min_pool_bytes);
    if (total < min_malloc || total - min_malloc < min_pool) {
        return l;
    }
    uint32_t room_for_pool = total - min_malloc;
    uint32_t pool = page_up(pool_bytes);
    pool = pool < min_pool ? min_pool : pool;
    l.pool_bytes = pool < room_for_pool ? pool : room_for_pool;
    l.malloc_bytes = total - l.pool_bytes;
    l.total_bytes = total;
    l.ok = true;
    return l;
}

MemLayout mem_layout_plan_or_tight(uint32_t available, uint32_t pool_bytes, uint32_t min_pool_bytes,
                                   uint32_t min_malloc_bytes, uint32_t tight_min_malloc_bytes)
{
    MemLayout l = mem_layout_plan(available, pool_bytes, min_pool_bytes, min_malloc_bytes);
    if (l.ok) {
        return l;
    }
    l = mem_layout_plan(available, min_pool_bytes, min_pool_bytes, tight_min_malloc_bytes);
    l.tight = l.ok;
    return l;
}
