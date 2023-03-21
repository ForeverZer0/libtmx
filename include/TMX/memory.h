#ifndef TMX_MEMORY_H
#define TMX_MEMORY_H

#include "TMX/typedefs.h"
#include <stddef.h>

// c-spell: disable

/**
 * @brief Prototype for a user-defined @c malloc function.
 *
 * @param size The number of bytes to allocate.
 * @param user The user-defined value assigned when the callback was set.
 *
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
typedef void *(*TMXmallocfunc)(size_t size, TMXuserptr user);

/**
 * @brief Prototype for a user-defined @c realloc function.
 *
 * @param previous The pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc or @ref tmxRealloc to be
 * reallocated. If this is @c NULL, a new block is allocated and a pointer to it is returned by the function.
 * @param newSize The new size for the memory block, in bytes. If it is @c 0 and @a previous points to an existing block of memory, the
 * memory block pointed by @a previous is deallocated and a @c NULL pointer is returned.
 * @param user The user-defined value assigned when the callback was set.
 *
 * @return A pointer to the newly allocated memory, or @c NULL if the request fails.
 */
typedef void *(*TMXreallocfunc)(void *previous, size_t newSize, TMXuserptr user);

/**
 * @brief Prototype for a user-defined @c calloc function.
 *
 * @param elemCount The number of elements to be allocated.
 * @param elemSize The size of a single element.
 * @param user The user-defined value assigned when the callback was set.
 *
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
typedef void *(*TMXcallocfunc)(size_t elemCount, size_t elemSize, TMXuserptr user);

/**
 * @brief Prototype for a user-defined @c free function.
 *
 * @param memory Pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc or @ref tmxRealloc to be deallocated. If
 * a null pointer is passed as argument, no action occurs.
 * @param user The user-defined value assigned when the callback was set.
 */
typedef void (*TMXfreefunc)(void *memory, TMXuserptr user);

/**
 * @brief Structure containing function pointers for memory allocation and freeing.
 */
typedef struct TMXallocator
{
    TMXmallocfunc malloc;   /** The user-defined `malloc` function. */
    TMXreallocfunc realloc; /** The user-defined `realloc` function. */
    TMXcallocfunc calloc;   /** The user-defined `calloc` function. */
    TMXfreefunc free;       /** The user-defined `free` function. */
} TMXallocator;

/**
 * @brief Assigns user-defined functions for allocating and freeing memory.
 *
 * @param allocator A structure containing the function pointers for memory allocation. All fields must be assigned.
 * @param user A user-defined pointer that will be supplied with each call.
 *
 * @return @ref TMX_TRUE on success, otherwise @ref TMX_FALSE.
 */
TMXbool tmxSetAllocator(TMXallocator allocator, TMXuserptr user);

/**
 * @brief Allocates the requested memory and returns a pointer to it.
 * @param size The size of the memory block, in bytes.
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
void *tmxMalloc(size_t size);

/**
 * @brief Attempts to resize the memory block pointed to by @a previous that was previously allocated with a call to @ref tmxMalloc or @ref
 * tmxCalloc.
 *
 * @param previous The pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc or @ref tmxRealloc to be
 * reallocated. If this is @c NULL, a new block is allocated and a pointer to it is returned by the function.
 * @param newSize The new size for the memory block, in bytes. If it is @c 0 and @a previous points to an existing block of memory, the
 * memory block pointed by @a previous is deallocated and a @c NULL pointer is returned.
 *
 * @return A pointer to the newly allocated memory, or @c NULL if the request fails.
 */
void *tmxRealloc(void *previous, size_t newSize);

/**
 * @brief Alocates the requested memory and returns a pointer to it, with all memory initialized to zero.
 *
 * @param elemCount The number of elements to be allocated.
 * @param elemSize The size of a single element.
 *
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
void *tmxCalloc(size_t elemCount, size_t elemSize);

/**
 * @brief Deallocates the memory previously allocated by a call to @ref tmxMalloc, @ref tmxCalloc, or @ref tmxRealloc
 * @param memory Pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc, or @ref tmxRealloc to be deallocated.
 * If a null pointer is passed as argument, no action occurs.
 */
void tmxFree(void *memory);

#endif /* TMX_MEMORY_H */