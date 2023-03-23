
#include "tmx/memory.h"
#include "tmx/error.h"
#include "internal.h"
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
static TMXuserptr userPtrValue;

TMX_INLINE void *
tmxMalloc(size_t size)
{
    if (!size)
        return NULL;
#ifdef TMX_DEBUG
    allocationCount++;
#endif
    void *ptr = memoryPool.malloc(size, userPtrValue);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

TMX_INLINE void *
tmxRealloc(void *previous, size_t newSize)
{
#ifdef TMX_DEBUG
    if (!previous)
        allocationCount++;
    else if (!newSize)
        deallocationCount++;
#endif

    void *ptr = memoryPool.realloc(previous, newSize, userPtrValue);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

TMX_INLINE void *
tmxCalloc(size_t elemCount, size_t elemSize)
{
#ifdef TMX_DEBUG
    allocationCount++;
#endif
    void *ptr = memoryPool.calloc(elemCount, elemSize, userPtrValue);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

TMX_INLINE void
tmxFree(void *memory)
{
#ifdef TMX_DEBUG
    if (memory)
        deallocationCount++;
#endif
    memoryPool.free(memory, userPtrValue);
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