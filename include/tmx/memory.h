#ifndef TMX_MEMORY_H
#define TMX_MEMORY_H

#include "types.h"

/**
 * @brief Prototype for a user-defined @c malloc function.
 *
 * @param[in] size The number of bytes to allocate.
 * @param[in] user The user-defined value assigned when the callback was set.
 *
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
typedef void *(*TMXmallocfunc)(size_t size, TMXuserptr user);

/**
 * @brief Prototype for a user-defined @c realloc function.
 *
 * @param[in] previous The pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc or @ref tmxRealloc to be
 * reallocated. If this is @c NULL, a new block is allocated and a pointer to it is returned by the function.
 * @param[in] newSize The new size for the memory block, in bytes. If it is @c 0 and @a previous points to an existing block of memory, the
 * memory block pointed by @a previous is deallocated and a @c NULL pointer is returned.
 * @param[in] user The user-defined value assigned when the callback was set.
 *
 * @return A pointer to the newly allocated memory, or @c NULL if the request fails.
 */
typedef void *(*TMXreallocfunc)(void *previous, size_t newSize, TMXuserptr user);

/**
 * @brief Prototype for a user-defined @c calloc function.
 *
 * @param[in] elemCount The number of elements to be allocated.
 * @param[in] elemSize The size of a single element.
 * @param[in] user The user-defined value assigned when the callback was set.
 *
 * @return A pointer to the allocated memory, or @c NULL if the request fails.
 */
typedef void *(*TMXcallocfunc)(size_t elemCount, size_t elemSize, TMXuserptr user);

/**
 * @brief Prototype for a user-defined @c free function.
 *
 * @param[in] memory Pointer to a memory block previously allocated with @ref tmxMalloc, @ref tmxCalloc or @ref tmxRealloc to be
 * deallocated. If a null pointer is passed as argument, no action occurs.
 * @param[in] user The user-defined value assigned when the callback was set.
 */
typedef void (*TMXfreefunc)(void *memory, TMXuserptr user);

/**
 * @brief Prototype for a function to load the contents from a path in a "virtual" filesystem.
 * 
 * @param[in] path The filesystem path that needs loaded.
 * @param[in] basePath An optional base path that the @a path is relative to. May be @c NULL.
 * @param[in] user The user-defined value assigned when the callback was set.
 * 
 * @return A null-terminated pointer to the contents of the file, or @c NULL if failed.
 */
typedef const char *(*TMXreadfunc)(const char *path, const char *basePath, TMXuserptr user);

/**
 * @brief Sets a callback that can be used to load a file from a "virtual" filesystem. 
 * 
 * @details This can be used to resolve paths that cannot be found, or read from documents embedded in
 * your project that don't actually exist in the filesystem or otherwise not located as defined in the TMX.
 * 
 * @param[in] read A callback that will be invoked to read the contents of a file.
 * @param[in] free The free function that will be called on the pointer when it is no longer needed, or @c NULL if it does not freed.
 * @param[in] user A user-defined pointer that will be supplied with each call.
 */
void tmxFileReadCallback(TMXreadfunc read, TMXfreefunc free, TMXuserptr user);

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
 * @param[in] allocator A structure containing the function pointers for memory allocation. All fields must be assigned.
 * @param[in] user A user-defined pointer that will be supplied with each call.
 *
 * @return @ref TMX_TRUE on success, otherwise @ref TMX_FALSE.
 */
TMXbool tmxMemoryAllocator(TMXallocator allocator, TMXuserptr user);

#ifndef TMX_LOG_ALLOCATIONS

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

#else

void *tmxMallocLogged(size_t size, const char *file, int line);
void *tmxReallocLogged(void *previous, size_t size, const char *file, int line);
void *tmxCallocLogged(size_t count, size_t size, const char *file, int line);
void tmxFreeLogged(void *ptr, const char *file, int line);

#define tmxMalloc(size) tmxMallocLogged((size), __FILE__, __LINE__)
#define tmxCalloc(count, size) tmxCallocLogged((count), (size), __FILE__, __LINE__)
#define tmxRealloc(previous, size) tmxReallocLogged((previous), (size), __FILE__, __LINE__)
#define tmxFree(ptr) tmxFreeLogged((ptr), __FILE__, __LINE__)

#endif

void tmxFreeMap(TMXmap *map);
void tmxFreeTileset(TMXtileset *tileset);
void tmxFreeTemplate(TMXtemplate *template);







#endif /* TMX_MEMORY_H */