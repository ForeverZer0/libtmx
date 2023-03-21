
#include "TMX/memory.h"
#include "TMX/error.h"
#include "utils.h"
#include <stdlib.h>

#ifdef TMX_DEBUG
static size_t allocationCount;
static size_t deallocationCount;
#endif

static void *
tmxMallocImpl(size_t size, TMXuserptr user)
{
    TMX_UNUSED(user);
    return malloc(size);
}

static void *
tmxReallocImpl(void *previous, size_t newSize, TMXuserptr user)
{
    TMX_UNUSED(user);
    return realloc(previous, newSize);
}

static void *
tmxCallocImpl(size_t elemCount, size_t elemSize, TMXuserptr user)
{
    TMX_UNUSED(user);
    return calloc(elemCount, elemSize);
}

static void
tmxFreeImpl(void *memory, TMXuserptr user)
{
    TMX_UNUSED(user);
    free(memory);
}

static TMXallocator memoryPool = {tmxMallocImpl, tmxReallocImpl, tmxCallocImpl, tmxFreeImpl};
static TMXuserptr userPtr;

inline void *
tmxMalloc(size_t size)
{
    void *ptr = memoryPool.malloc(size, userPtr);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

inline void *
tmxRealloc(void *previous, size_t newSize)
{
#ifdef TMX_DEBUG
    if (!previous)
        allocationCount++;
    else if (!newSize)
        deallocationCount++;
#endif

    void *ptr = memoryPool.realloc(previous, newSize, userPtr);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

inline void *
tmxCalloc(size_t elemCount, size_t elemSize)
{
#ifdef TMX_DEBUG
    allocationCount++;
#endif
    void *ptr = memoryPool.calloc(elemCount, elemSize, userPtr);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

inline void
tmxFree(void *memory)
{
#ifdef TMX_DEBUG
    if (memory)
        deallocationCount++;
#endif
    memoryPool.free(memory, userPtr);
}

#ifdef TMX_DEBUG

#include <stdio.h>

#define BLUE   "\033[34m"
#define YELLOW "\033[33m"
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define BRIGHT "\033[1m"
#define RESET  "\033[0m"

void
tmxMemoryLeakCheck(void)
{
    printf("\n" BLUE ":: " RESET BRIGHT "Leak Check\n" RESET);
    printf(YELLOW " -> " RESET "Allocations:    %5zu\n", allocationCount);
    printf(YELLOW " -> " RESET "Deallocations:  %5zu\n", deallocationCount);
    printf(YELLOW " -> " RESET "Result:          " BRIGHT "%s\n\n" RESET, allocationCount == deallocationCount ? GREEN "PASS" : RED "FAIL");
}
#endif