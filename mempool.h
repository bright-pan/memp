#ifndef MEMPOOL_H_
#define MEMPOOL_H_

#include <stdbool.h>
#include <stdint.h>
#include <aos/kernel.h>

#define SRAM_RTMP 0x00
#define SRAM_COMMON 0x01
#define SRAM_EX0 0x02
#define SRAM_EX1 0x03
#define SRAM_EX2 0x04
#define SRAM_BANK (SRAM_EX2 + 1)

#define INSRAM  // __attribute__((at(0x30000000 + 0x00000000)));
#define EXTRAM  // __attribute__((at(0x40000000 + 0x00000000)));
#define CCMRAM  // __attribute__((at(0x50000000 + 0x00000000)));

#define ALIGN_SIZE  // __align(4)

#define SRAM_RTMP_NAME "SRAM_RTMP"
#define SRAM_RTMP_BLOCK_SIZE 32
#define SRAM_RTMP_MEM_SIZE  3 * 1024 * 1024

#define SRAM_COMMON_NAME "SRAM_COMMON"
#define SRAM_COMMON_BLOCK_SIZE 32
#define SRAM_COMMON_MEM_SIZE  4 * 1024 * 1024

#define SRAM_EX0_NAME "SRAM_EX0"
#define SRAM_EX0_BLOCK_SIZE 32
#define SRAM_EX0_MEM_SIZE 1 * 1024

#define SRAM_EX1_NAME "SRAM_EX1"
#define SRAM_EX1_BLOCK_SIZE 32
#define SRAM_EX1_MEM_SIZE 1 * 1024

#define SRAM_EX2_NAME "SRAM_EX2"
#define SRAM_EX2_BLOCK_SIZE 32
#define SRAM_EX2_MEM_SIZE 1 * 1024

#define MEMPOOL_INIT_READY 0
#define MEMPOOL_INIT_DONE 1
#define MEMPOOL_NAME_SIZE 20

typedef struct {
    char name[MEMPOOL_NAME_SIZE+1];
    uint8_t *mem; // memory address
    uint32_t msize; // memory size
    uint16_t *table; // table address
    uint32_t tsize; // table size
    uint32_t bsize; // block size
    uint8_t ready;
    uint32_t used;
    uint32_t peak;
    aos_mutex_t mutex;
} MEMPOOL_T;

#define MEMPOOL_MALLOC(memx, size) mempool_malloc((memx), (size), __FILE__, __LINE__)
#define MEMPOOL_CALLOC(memx, n, size) mempool_calloc((memx), (n), (size), __FILE__, __LINE__)
#define MEMPOOL_REALLOC(memx, ptr, size) mempool_realloc((memx), (ptr), (size), __FILE__, __LINE__)
#define MEMPOOL_FREE(ptr) mempool_free((ptr), __FILE__, __LINE__)

MEMPOOL_T *get_mempool(uint8_t memx);
void mempool_init(void);
int mempool_create(uint8_t memx, char *name, uint32_t msize, uint32_t bsize);
int mempool_destroy(uint8_t memx);

void* mempool_malloc(uint8_t memx, uint32_t size, char* file_name, uint32_t func_line);
void* mempool_calloc(uint8_t memx, uint32_t nitems, uint32_t size, char* file_name, uint32_t func_line);
void* mempool_realloc(uint8_t memx, void* ptr, uint32_t size, char* file_name, uint32_t func_line);
void mempool_free(void* ptr, char* file_name, uint32_t func_line);

void mempool_memset(void* src, uint8_t c, uint32_t count);
void mempool_memcpy(void* des, void* src, uint32_t size);

/* just for debug, print memory use information */
float get_mempool_used_percent(uint8_t memx);
float get_mempool_peak_percent(uint8_t memx);
uint32_t get_mempool_peak(uint8_t memx);
uint32_t get_mempool_size(uint8_t memx);

void mempool_trace(void);

#endif  // MEMPOOL_H_
