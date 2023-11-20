#include "mempool.h"
#include <stddef.h>
#include <stdio.h>
#include "include/mempool.h"


#define CONFIG_MEMORY_POOL_DEBUG 1

#if CONFIG_MEMORY_POOL_DEBUG
#include "mempool_debug.h"
#endif

// configure log
// #define CONFIG_LANGO_DBG_LEVEL LANGO_DBG_LOG
#define LANGO_DBG_TAG "MEM_POOL"
#include "lango_log.h"
// ------------------


static MEMPOOL_T mempool[SRAM_BANK];

/*
static INSRAM ALIGN_SIZE uint8_t mem1pool[MEM1_POOL_SIZE] = {0};
static EXTRAM ALIGN_SIZE uint8_t mem2pool[MEM2_POOL_SIZE] = {0};
static CCMRAM ALIGN_SIZE uint8_t mem3pool[MEM3_POOL_SIZE] = {0};
static EXTRAM ALIGN_SIZE uint8_t mem4pool[MEM4_POOL_SIZE] = {0};
static EXTRAM ALIGN_SIZE uint8_t mem5pool[MEM5_POOL_SIZE] = {0};

static INSRAM uint16_t mem1table[MEM1_TABLE_SIZE] = {0};
static EXTRAM uint16_t mem2table[MEM2_TABLE_SIZE] = {0};
static CCMRAM uint16_t mem3table[MEM3_TABLE_SIZE] = {0};
static EXTRAM uint16_t mem4table[MEM4_TABLE_SIZE] = {0};
static EXTRAM uint16_t mem5table[MEM5_TABLE_SIZE] = {0};

static const uint32_t memtablesize[SRAMBANK] = {
    MEM1_TABLE_SIZE, MEM2_TABLE_SIZE, MEM3_TABLE_SIZE, MEM4_TABLE_SIZE,
    MEM5_TABLE_SIZE};

static const uint32_t memblocksize[SRAMBANK] = {
    MEM1_BLOCK_SIZE, MEM2_BLOCK_SIZE, MEM3_BLOCK_SIZE, MEM4_BLOCK_SIZE,
    MEM5_BLOCK_SIZE};

static const uint32_t mempoolsize[SRAMBANK] = {MEM1_POOL_SIZE, MEM2_POOL_SIZE,
                                               MEM3_POOL_SIZE, MEM4_POOL_SIZE
                                               MEM5_POOL_SIZE};


static MALLOC_DEV_T malloc_dev = {
    mempool_init,
    mem_perused,
    {0, }
};

*/

MEMPOOL_T *get_mempool(uint8_t memx) {
    MEMPOOL_T *pool = NULL;
    if (memx < SRAM_BANK) {
        pool = &mempool[memx];
    }
    return pool;
}

static void mutex_creat(uint8_t memx) {
    aos_mutex_new(&mempool[memx].mutex);
}

static void mutex_destroy(uint8_t memx) {
    aos_mutex_free(&mempool[memx].mutex);
}

static void mutex_lock(uint8_t memx) {
    aos_mutex_lock(&mempool[memx].mutex, AOS_WAIT_FOREVER);
}

static void mutex_unlock(uint8_t memx) {
    aos_mutex_unlock(&mempool[memx].mutex);
}

int mempool_create(uint8_t memx, char *name, uint32_t msize, uint32_t bsize) {
    mutex_lock(memx);
    int ret = -1;
    MEMPOOL_T *pool = get_mempool(memx);

    if (pool) {
        pool->table = aos_malloc(sizeof(uint16_t) * (msize / bsize));
        pool->mem = aos_malloc(msize);
        if (pool->table && pool->mem) {
            strncpy(pool->name, name, MEMPOOL_NAME_SIZE);
            pool->msize = msize;
            pool->bsize = bsize;
            pool->tsize = msize / bsize;
            memset(pool->table, 0, pool->tsize);
            memset(pool->mem, 0, pool->msize);
            pool->ready = MEMPOOL_INIT_DONE;
            mutex_creat(memx);
            ret = 0;
        } else {
            // free table
            if (pool->table) {
                aos_free(pool->table);
            }
            // free mem
            if (pool->mem) {
                aos_free(pool->mem);
            }
        }
    }
    mutex_unlock(memx);
    return ret;
}

int mempool_destroy(uint8_t memx) {
    mutex_lock(memx);
    int ret = -1;
    MEMPOOL_T *pool = get_mempool(memx);

    if (pool) {
        if (pool->used) {
            LANGO_LOG_ERR("mempool %s,%d has been used", pool->name, memx);
        } else {
            mutex_destroy(memx);
            aos_free(pool->table);
            aos_free(pool->mem);
            pool->ready = MEMPOOL_INIT_READY;
            ret = 0;
        }
    }
    mutex_lock(memx);
    return ret;
}

void mempool_init(void) {
    for (uint8_t i=0; i < SRAM_BANK; i++) {
        memset(&mempool[i], 0, sizeof(MEMPOOL_T));
    }

    if (mempool_create(SRAM_RTMP, SRAM_RTMP_NAME, SRAM_RTMP_MEM_SIZE, SRAM_RTMP_BLOCK_SIZE) < 0) {
        LANGO_LOG_ERR("mempool create error: %d, %s, %d, %d", SRAM_RTMP, SRAM_RTMP_NAME,
                      SRAM_RTMP_MEM_SIZE, SRAM_RTMP_BLOCK_SIZE);
    }
    if (mempool_create(SRAM_COMMON, SRAM_COMMON_NAME, SRAM_COMMON_MEM_SIZE, SRAM_COMMON_BLOCK_SIZE) < 0) {
        LANGO_LOG_ERR("mempool create error: %d, %s, %d, %d", SRAM_RTMP, SRAM_COMMON_NAME,
                      SRAM_COMMON_MEM_SIZE, SRAM_COMMON_BLOCK_SIZE);
    }
#if CONFIG_MEMORY_POOL_DEBUG
    memory_pool_debug_init();
#endif
}

static inline void _cal_mempool_used(uint8_t memx) {
    uint32_t used = 0;

    MEMPOOL_T *pool = get_mempool(memx);
    for (uint32_t i = 0; i < pool->tsize; i++) {
        if (pool->table[i]) {
            used++;
        }
    }
    pool->used = used * pool->bsize;
}

static inline void _cal_mempool_peak(uint8_t memx) {
    uint32_t used = 0;

    MEMPOOL_T *pool = get_mempool(memx);
    for (uint32_t i = 0; i < pool->tsize; i++) {
        if (pool->table[i]) {
            used++;
        }
    }
    pool->used = used * pool->bsize;
    if (pool->peak < pool->used) {
        pool->peak = pool->used;
    }
}

uint32_t get_mempool_peak(uint8_t memx) {
    return get_mempool(memx)->peak;
}

uint32_t get_mempool_size(uint8_t memx) {
    return get_mempool(memx)->msize;
}

float get_mempool_peak_percent(uint8_t memx) {
    MEMPOOL_T *pool = get_mempool(memx);
    return (pool->peak * 100.0f) / pool->msize;
}

float get_mempool_used_percent(uint8_t memx) {
    MEMPOOL_T *pool = get_mempool(memx);
    return (pool->used * 100.0f) / pool->msize;
}

static inline uint16_t _count_need_block_nums(uint8_t memx, uint32_t size) {
    MEMPOOL_T *pool = get_mempool(memx);
    uint16_t need_block_nums = size / pool->bsize;
    if (size % pool->bsize) {
        need_block_nums++;
    }
    return need_block_nums;
}

static uint32_t mempool_mem_malloc(uint8_t memx, uint32_t size) {
    MEMPOOL_T *pool = get_mempool(memx);

    if (pool == NULL || pool->ready != MEMPOOL_INIT_DONE) {
        LANGO_LOG_ERR("mempool %s,%d is not init done!", pool->name, memx);
        goto MALLOC_ERROR;
    }
    if (size == 0 || pool->msize < size) {
        LANGO_LOG_ERR("mempool %s,%d malloc size error: 0 < size <= %d",
                      pool->name, memx, pool->msize);
        goto MALLOC_ERROR;
    }

    uint16_t need_block_nums = _count_need_block_nums(memx, size);
    //LANGO_LOG_INFO("need block nums : %d", need_block_nums);
    uint16_t empty_block_count = 0;
    for (int32_t offset = (pool->tsize - 1); offset >= 0; offset--) {
        if (offset < 0) {
            //return 0xffffffff;
            LANGO_LOG_ERR("offset < 0, no enough block num : %d", need_block_nums);
            goto MALLOC_ERROR;
        }

        empty_block_count = (pool->table[offset] == 0)
                               ? (empty_block_count + 1)
                               : 0;

        if (empty_block_count == need_block_nums) {
            for (uint32_t i = 0; i < need_block_nums; i++) {
                pool->table[offset + i] = need_block_nums;
            }
            _cal_mempool_peak(memx); // cal used peak
            /* offset address */
            return (offset * pool->bsize);
        }
    }

    LANGO_LOG_ERR("no enough block num : %d", need_block_nums);
MALLOC_ERROR:
    return 0xffffffff;
}

static uint8_t mempool_mem_free(uint8_t memx, uint32_t offset) {
    MEMPOOL_T *pool = get_mempool(memx);
    if (pool && pool->ready == MEMPOOL_INIT_DONE) {
        if (offset < pool->msize) {
            int index = offset / pool->bsize;
            int nmemb = pool->table[index];
            for (uint16_t i = 0; i < nmemb; i++) {
                pool->table[index + i] = 0;
            }
            _cal_mempool_used(memx);
            return 0;
        } else {
            LANGO_LOG_ERR("free error offset > poolsize: memx=%d, offset=%d, poolsize=%d", memx, offset, pool->msize);
        }
        return 2;
    } else {
        LANGO_LOG_ERR("mempool %s,%d is not init done!", pool->name, memx);
        return 1;
    }
}

void mempool_memcpy(void* des, void* src, uint32_t n) {
    uint8_t* p_des = des;
    uint8_t* p_src = src;
    while (n--) {
        *p_des++ = *p_src++;
    }
}

void mempool_memset(void* src, uint8_t c, uint32_t count) {
    uint8_t* p_des = src;
    while (count--) {
        *p_des++ = c;
    }
}

void mempool_free(void* ptr, char* file_name, uint32_t func_line) {
    MEMPOOL_T *pool = NULL;
    uint8_t memx = 0xff;

    if (ptr != NULL) {
        for (uint8_t i = 0; i < SRAM_BANK; i++) {
            pool = get_mempool(i);
            if (pool && pool->ready == MEMPOOL_INIT_DONE &&
                (uintptr_t)ptr >= (uintptr_t)pool->mem &&
                (uintptr_t)ptr < (uintptr_t)pool->mem + pool->msize) {
                memx = i;
                break;
            } else {
                //LANGO_LOG_ERR("ptr is not in mempool %s: 0x%08p < 0x%08p < 0x%08p",
                //              pool->name, pool->mem, ptr, pool->mem+pool->msize);
            }
        }
    }
    if (memx != 0xff) {
        mutex_lock(memx);
        uint32_t offset = (uintptr_t)ptr - (uintptr_t)pool->mem;
        mempool_mem_free(memx, offset);
#if CONFIG_MEMORY_POOL_DEBUG
        memory_pool_debug_del(ptr, file_name, func_line);
#endif
        mutex_unlock(memx);
    } else {
        LANGO_LOG_ERR("free error, %s(%d) ptr is not in mempool: 0x%08p", file_name, func_line, ptr);
    }
}

void* mempool_malloc(uint8_t memx, uint32_t size, char* file_name, uint32_t func_line) {
    mutex_lock(memx);
    void* addr = NULL;
    MEMPOOL_T *pool = get_mempool(memx);
    if (pool && pool->ready == MEMPOOL_INIT_DONE) {
        uint32_t offset = mempool_mem_malloc(memx, size);
        if (offset != 0xffffffff) {

            addr = (void*)((uintptr_t)pool->mem + offset);

#if CONFIG_MEMORY_POOL_DEBUG
            memory_pool_debug_add(memx, size, addr, file_name, func_line);
#endif
        } else {
            LANGO_LOG_ERR("malloc error, %s(%d) for memx=%d, size=%d", file_name, func_line, memx, size);
        }
    } else {
        LANGO_LOG_ERR("mempool %s,%d is not init done!", pool->name, memx);
    }
    mutex_unlock(memx);
    return addr;
}

void* mempool_calloc(uint8_t memx, uint32_t nitems, uint32_t size, char* file_name, uint32_t func_line) {
    mutex_lock(memx);
    void* addr = NULL;
    addr = mempool_malloc(memx, nitems * size, file_name, func_line);
    if (addr != NULL) {
        mempool_memset(addr, 0, nitems * size);
    }
    mutex_unlock(memx);
    return addr;
}

void* mempool_realloc(uint8_t memx, void* ptr, uint32_t size, char* file_name, uint32_t func_line) {
    void* addr = NULL;
    MEMPOOL_T *pool = get_mempool(memx);

    if (pool && pool->ready == MEMPOOL_INIT_DONE) {
        if (ptr != NULL) {
            if ((uintptr_t)ptr > (uintptr_t)pool->mem &&
                (uintptr_t)ptr < ((uintptr_t)pool->mem + pool->msize)) {
                mutex_lock(memx);
                uint32_t offset = mempool_mem_malloc(memx, size);
                if (offset != 0xffffffff) {
                    addr = (void*)((uintptr_t)pool->mem + offset);

#if CONFIG_MEMORY_POOL_DEBUG
                    memory_pool_debug_add(memx, size, addr, file_name, func_line);
#endif
                    uint32_t ptr_index = ((uintptr_t)ptr - (uintptr_t)pool->mem) / pool->bsize;
                    uint32_t ptr_size = pool->table[ptr_index] * pool->bsize;
                    uint32_t malloc_size = _count_need_block_nums(memx, size) * pool->bsize;
                    //LANGO_LOG_WRN("ptr=0x%08p, ptr_index=%d, ptr_size=%d, malloc_size=%d", ptr, ptr_index, ptr_size, malloc_size);
                    memset(addr, 0, malloc_size);
                    memcpy(addr, ptr, ptr_size > malloc_size ? malloc_size : ptr_size);
                    MEMPOOL_FREE(ptr);
                    ptr = NULL;
                } else {
                    LANGO_LOG_ERR("realloc error, %s(%d) for ptr=0x%08p, memx=%d, size=%d", file_name, func_line, ptr, memx, size);
                }
                mutex_unlock(memx);
            } else {
                LANGO_LOG_ERR("realloc error, %s(%d) ptr is not in mempool: 0x%08p", file_name, func_line, ptr);
            }
        } else {
            addr = MEMPOOL_MALLOC(memx, size);
        }
    } else {
        LANGO_LOG_ERR("mempool %s,%d is not init done!", pool->name, memx);
    }
    return addr;
}

void mempool_trace(void) {
    int32_t total;
    int32_t used;
    int32_t mfree;
    int32_t peak;

#if CONFIG_MEMORY_POOL_DEBUG
    memory_pool_debug_trace();
#endif
    aos_get_mminfo(&total, &used, &mfree, &peak);
    LANGO_LOG_DBG("mminfo: total=%d, used=%d, mfree=%d, peak=%d", total, used, mfree, peak);
}

#if 1

static void exit_function(void)
{
    LANGO_LOG_INFO("exit function run start");

#if CONFIG_MEMORY_POOL_DEBUG
    memory_pool_debug_trace();
    LANGO_LOG_INFO("malloc free count: %d", memory_pool_debug_malloc_free_count());
#endif

    LANGO_LOG_INFO("exit function run end");

}

static void mp_malloc(int argc, char **argv) {
    if (argc != 3) {
        LANGO_LOG_ERR("mempool_malloc n size");
    }
    uint8_t* ptr = MEMPOOL_MALLOC(atoi(argv[1]), atoi(argv[2]));
    if (ptr) {
        LANGO_LOG_INFO("ptr malloc ok, addr: %p", ptr);
        LANGO_LOG_INFO("ptr memory use info: %d%%", get_mempool_used_percent(atoi(argv[1])));
    } else {
        LANGO_LOG_INFO("ptr malloc fail");
    }
    if (ptr) {
        MEMPOOL_FREE(ptr);
        LANGO_LOG_INFO("ptr free ok");
        LANGO_LOG_INFO("ptr memory use info: %d%%", get_mempool_used_percent(atoi(argv[1])));
    }
}

static void mp_test(int argc, char **argv) {

    uint8_t* ptr = MEMPOOL_MALLOC(SRAM_COMMON, 12);
    if (ptr) {
        LANGO_LOG_INFO("ptr malloc ok, addr: %p", ptr);
        LANGO_LOG_INFO("ptr memory use info: %d%%", get_mempool_used_percent(SRAM_COMMON));
    } else {
        LANGO_LOG_INFO("ptr malloc fail");
    }

    uint8_t* ptr1 = MEMPOOL_MALLOC(SRAM_COMMON, 10);
    if (ptr1) {
        LANGO_LOG_INFO("ptr1 malloc ok, addr: %p", ptr1);
        LANGO_LOG_INFO("ptr1 memory use info: %d%%", get_mempool_used_percent(SRAM_COMMON));

    } else {
        LANGO_LOG_INFO("ptr1 malloc fail");

    }
    LANGO_LOG_INFO("--------------------------------------------------------");
    //for (uint8_t i = 0; i < 6; i++) {
    {
        uint8_t memx = SRAM_EX1;
        MEMPOOL_T *pool = get_mempool(memx);
        uint8_t *unused_ptr = MEMPOOL_MALLOC(memx, 200);
        if (unused_ptr == NULL) {
            LANGO_LOG_INFO("malloc fail");
        } else {
            memset(unused_ptr, 0xff, 200);
            LANGO_LOG_INFO("malloc : %p", unused_ptr);
            LANGO_LOG_INFO_DUMP_HEX("unsed_ptr: ", unused_ptr, 200);
            uint32_t _index = ((uintptr_t)unused_ptr - (uintptr_t)pool->mem) / pool->bsize;
            uint32_t _size = pool->table[_index] * pool->bsize;
            LANGO_LOG_WRN("ptr=%p, _index=%d, _size=%d", unused_ptr, _index, _size);
        }
        LANGO_LOG_INFO("--------------------------------------------------------");
        for (uint8_t i = 1; i < 6; i++) {
            uint32_t size = 100*i;
            unused_ptr = MEMPOOL_REALLOC(memx, unused_ptr, size);
            if (unused_ptr == NULL) {
                LANGO_LOG_INFO("malloc fail");
            } else {
                LANGO_LOG_INFO("malloc : %p", unused_ptr);
                LANGO_LOG_INFO_DUMP_HEX("unsed_ptr: ", unused_ptr, size);
                uint32_t _index = ((uintptr_t)unused_ptr - (uintptr_t)pool->mem) / pool->bsize;
                uint32_t _size = pool->table[_index] * pool->bsize;
                LANGO_LOG_WRN("ptr=%p, _index=%d, _size=%d", unused_ptr, _index, _size);
            }
        }
    }
    uint8_t* refree_ptr = ptr;
    MEMPOOL_FREE(ptr);
    MEMPOOL_FREE(refree_ptr);

    exit_function();
}

static void mp_trace(int argc, char **argv) {
    mempool_trace();
}

ALIOS_CLI_CMD_REGISTER(mp_test, mp_test, mp_test);
ALIOS_CLI_CMD_REGISTER(mp_malloc, mp_malloc, mp_malloc);
ALIOS_CLI_CMD_REGISTER(mp_trace, mp_trace, mp_trace);
#endif
