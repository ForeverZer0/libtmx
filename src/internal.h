/**
 * @file internal.h
 * @brief Contains common helper macros and functions that are not part of the public API.
 */
#ifndef TMX_UTILS_H
#define TMX_UTILS_H

#include "tmx.h"
#include <stddef.h>



#include "tmx/memory.h"
#define uthash_malloc(sz)    tmxMalloc(sz)
#define uthash_free(ptr, sz) tmxFree(ptr)
#include "uthash.h"

struct TMXproperties
{
    const char *key;
    TMXproperty value;
    UT_hash_handle hh;
};

/**
 * @brief Allocates an object of the specified @a type with zeroed memory.
 * @param[in] type The type to allocate.
 * @return A pointer to the newly allocated object.
*/
#define TMX_ALLOC(type) ((type *) tmxCalloc(1, sizeof(type)))



/**
 * @brief Tests for the presence of a flag in a bitfield.
 * @param value The value to test.
 * @param flag The flag to test the presence of.
 * @return A boolean result.
 */
#define TMX_FLAG(value, flag) (((value) & (flag)) != 0)

/**
 * @brief Make your compiler be quiet.
 */
#define TMX_UNUSED(x) ((void) x)

/**
 * @brief Emits an error of the specified type with a generic error message.
 * @param errno An error code indicating the general type of error that occurred.
 */
void tmxError(TMXenum errno);

/**
 * @brief Emits an error of the specified type and supplies a brief message describing it.
 * @param errno An error code indicating the general type of error that occurred.
 * @param message The message to supply with the error.
 */
void tmxErrorMessage(TMXenum errno, const char *message);

/**
 * @brief Emits an error of the specified type and supplies a brief message describing it.
 * @param errno An error code indicating the general type of error that occurred.
 * @param message A @c sprintf style format message to supply with the error.
 * @param ... Arguments for the format string.
 */
void tmxErrorFormat(TMXenum errno, const char *format, ...);

/**
 * Allocates and copies a string.
 * @param[in] input The string to duplicate.
 * @param[in] inputSize The length of the @a input string to copy, or @c 0 to have input measured with @c strlen.
 * @return A duplicate of the string, or @c NULL when @a input is @c NULL.
 * @note The returned result will be null-terminated.
 */
char *tmxStringCopy(const char *input, size_t inputSize);

/**
 * @brief Allocates and copies a zero-terminated string.
 * @param[in] input The string to duplicate.
 * @return A duplicate of the string, or @c NULL when @a input is @c NULL.
 * @note The returned result will be null-terminated.
 */
#define tmxStringDup(input) tmxStringCopy(input, 0)

/**
 * @brief If defined, invokes the user-callback for image loading.
 *
 * @param[in] image The image that is loaded.
 * @param[in] basePath An optional base path that the image source is relative.
 */
void tmxImageUserLoad(TMXimage *image, const char *basePath);

/**
 * @brief If defined, invokes the user-callback to free an image.
 *
 * @param[in] image The image to free.
 */
void tmxImageUserFree(TMXimage *image);

/**
 * @brief Reads the contents of a file into a buffer.
 * 
 * @param[in] path The given path of the file.
 * @param[in] basePath An optional base path the @a path is relative to.
 * @return A buffer containing the contents of the file. The caller is responsible for freeing with @ref tmxFree.
 */
char *tmxFileRead(const char *path, const char *basePath);

/**
 * @brief Appends a value to an array, resizing as needed.
 * 
 * @param T The type used in the array. 
 * @param array The pointer to array of type @a T.
 * @param object An object of type @a T.
 * @param count The current number of items in the @a array.
 * @param capacity The current capacity of the @a array.
 * 
 * @note The @a array pointer, @a count, @a capacity may be modified.
 */
#define tmxArrayPush(T, array, object, count, capacity)                                                                                    \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (count >= capacity)                                                                                                             \
        {                                                                                                                                  \
            capacity *= 2;                                                                                                                 \
            array = (T *) tmxRealloc(array, capacity * sizeof(T));                                                                         \
        }                                                                                                                                  \
        array[count++] = object;                                                                                                           \
    } while (0)

/**
 * @brief Marks the updating of an array as complete, trimming excess capacity as needed.
 * @param T The type used in the array. 
 * @param array The pointer to array of type @a T.
 * @param count The current number of items in the @a array.
 * @param capacity The current capacity of the @a array.
 */
#define tmxArrayFinish(T, array, count, capacity)                                                                                          \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (count < capacity)                                                                                                              \
            array = (T *) tmxRealloc(array, count * sizeof(T));                                                                            \
    } while (0)


/**
 * @brief Update the values not explicitly defined to reflect those of a template object.
 * 
 * @param[in] dst The object to merge to.
 * @param[in] src The source object to copy values from.
 */
void tmxObjectMergeTemplate(TMXobject *dst, TMXobject *src);

/**
 * @brief Creates a deep-copy of the properties.
 *
 * @param[in] properties The properties to duplicate.
 * @return The newly allocated cloned properties.
 */
TMXproperties *tmxPropertiesDup(TMXproperties *properties);

/**
 * @brief Merges two property hashes together. When the same key is found in both hashes, the @a dst hash
 * will be retain its value and not be overwritten.
 * 
 * @param dst The destination properties to merge to, or @c NULL to create a deep-copy of @a src.
 * @param src The source properties to merge.
 * 
 * @return The @a dst properties merged with @a src, or a copy of @a src if @a dst was @c NULL.
 */
TMXproperties * tmxPropertiesMerge(TMXproperties *dst, TMXproperties *src);

#endif /* TMX_UTILS_H */