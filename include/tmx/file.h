#ifndef TMX_FILE_H
#define TMX_FILE_H

#include "types.h"

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
 * @brief Returns the size of of subsection that makes up the directory portion of the given @a path.
 *
 * @param[in] path The filesystem path to query.
 * @return The length of directory portion (including final path separator), or @c 0 if unable to determine.
 */
size_t tmxFileDirectory(const char *path);

/**
 * @brief Builds an absolute path from two paths: a relative path, and the an optional base path it is relative to.
 * 
 * @param[in] path The path to ensure is absolute. May be relative or absolute.
 * @param[in] basePath An optional base path that @a path is relative to.
 * @param[in,out] buffer A buffer to receive the resulting absolute path.
 * @param[in] bufferSize The maximum number of bytes that can be written to the buffer, including the null-terminator.
 * @return The number of bytes written to the @a buffer, excluding the null-terminator.
 * @note The @a buffer is guaranteed to be null-terminated.
 */
size_t tmxFileAbsolutePath(const char *path, const char *basePath, char *buffer, size_t bufferSize);

#endif /* TMX_FILE_H */