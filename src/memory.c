
#include <stddef.h>
#include <stdlib.h>

// struct TMXmemorypool
// {

// }

void *
tmxMalloc(size_t size)
{
    return malloc(size);
}

void *
tmxRealloc(void *previous, size_t newSize)
{
    return realloc(previous, newSize);
}

void *
tmxCalloc(size_t numElem, size_t elemSize)
{
    return calloc(numElem, elemSize);
}

void
tmxFree(void *memory)
{
    free(memory);
}