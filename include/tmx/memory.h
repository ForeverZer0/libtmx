/**
 * @file memory.h
 * @author Eric Freed
 * @brief Provides functions for memory allocation and deallocation.
 * @version 0.1
 * @date 2023-03-28
 * 
 * @details To provide your own memory allocators, the following macros can be defined prior to including any headers
 * of this library.
 * 
 * * TMX_MALLOC(size, user)
 * * TMX_CALLOC(elemCount, elemSize, user)
 * * TMX_REALLOC(previous, size, user)
 * * TMX_FREE(ptr, user)
 * 
 * At a bare minimum, the @c TMX_REALLOC macro must be defined if you wish to provide your own allocations, as the other
 * macros will automatically have their functionality provided through it if they are not defined. That stated, it 
 * is optimal to provide them all if your implementation has them available. It is also noteworthy that freeing 
 * memory with the use of only @c realloc is not considered portable, and typically associated with glib. 
 * 
 * You really want to at least provide @c TMX_REALLOC and @c TMX_FREE in most scenarios.
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef TMX_MEMORY_H
#define TMX_MEMORY_H

#include "common.h"

/**
 * @brief Assigns a user-pointer that will be passed to custom memory allocation/deallocation functions.
 *
 * @param[in] user A user-defined pointer that will be supplied with each call.
 */
void tmxMemoryUserPtr(TMXuserptr user);

/**
 * @brief Allocates the requested memory and returns a pointer to it.
 * @param[in] size The size of the memory block, in bytes.
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
void *tmxMalloc(size_t size);

/**
 * @brief Attempts to resize the memory block pointed to by @a previous that was previously allocated with a call to @ref tmxMalloc or @ref
 * tmxCalloc.
 *
 * @param[in] previous The pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc or @ref tmxRealloc to be
 * reallocated. If this is @c NULL, a new block is allocated and a pointer to it is returned by the function.
 * @param[in] newSize The new size for the memory block, in bytes. If it is @c 0 and @a previous points to an existing block of memory, the
 * memory block pointed by @a previous is deallocated and a @c NULL pointer is returned.
 *
 * @return A pointer to the newly allocated memory, or @c NULL if the request fails.
 */
void *tmxRealloc(void *previous, size_t newSize);

/**
 * @brief Allocates the requested memory and returns a pointer to it, with all memory initialized to zero.
 *
 * @param[in] elemCount The number of elements to be allocated.
 * @param[in] elemSize The size of a single element.
 *
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
void *tmxCalloc(size_t elemCount, size_t elemSize);

/**
 * @brief Deallocates the memory previously allocated by a call to @ref tmxMalloc, @ref tmxCalloc, or @ref tmxRealloc
 * @param[in] memory Pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc, or @ref tmxRealloc to be
 * deallocated. If a null pointer is passed as argument, no action occurs.
 */
void tmxFree(void *memory);

#endif /* TMX_MEMORY_H */