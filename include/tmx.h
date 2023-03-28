#ifndef TMX_H
#define TMX_H

#if defined(_WIN32) || defined(__CYGWIN__)
#define TMX_EXPORT __declspec(dllexport)
#define TMX_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
#define TMX_EXPORT __attribute__((visibility("default")))
#define TMX_IMPORT __attribute__((visibility("default")))
#else
#define TMX_EXPORT
#define TMX_IMPORT
#endif

#if defined(TMX_SHARED)
#if defined(TMX_EXPORTS)
#define TMX_PUBLIC TMX_EXPORT
#else
#define TMX_PUBLIC TMX_IMPORT
#endif
#else
#define TMX_PUBLIC
#endif

#ifndef TMX_ASSERT
#include <assert.h>
#define TMX_ASSERT(expr) assert(expr)
#endif

#include "tmx/types.h"

#define TMX_GID_CLEAN(x) ((x) &TMX_GID_MASK)
#define TMX_GID_FLAGS(x) ((x) & ~TMX_GID_MASK)


/**
 * @brief Returns the minumum of two comparable values.
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return The minimum of the two values.
 * @warning Be wary of possible double-evaluation for the inputs.
 * @note This macro is not part of the "official" API, but is included publicly for convenience.
 */
#define TMX_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Returns the maximum of two comparable values.
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return The minimum of the two values.
 * @warning Be wary of possible double-evaluation for the inputs.
 * @note This macro is not part of the "official" API, but is included publicly for convenience.
 */
#define TMX_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Returns a value clamped between a minimum and maximum.
 * @param x The value to clamp.
 * @param min The minimum value.
 * @param max The maximum value.
 * @return The clamped value.
 * @warning Be wary of possible double-evaluation for the inputs.
 * @note This macro is not part of the "official" API, but is included publicly for convenience.
 */
#define TMX_CLAMP(x, min, max) TMX_MAX(min, TMX_MIN(max, x))

/**
 * @brief Prototype for error callbacks.
 *
 * @param errno An error code indicating the general type of error that occurred.
 * @param message A brief description of the error.
 * @param user The user-defined pointer that was specified when setting the callback.
 *
 */
typedef void (*TMXerrorfunc)(TMXenum errno, const char *message, TMXuserptr user);

/**
 * @brief Sets a callback function that will be invoked when errors are emitted by the library.
 *
 * @param callback The function to invoke when an error occurs.
 * @param user A user-defined pointer that will be passed to the callback function.
 */
void tmxErrorCallback(TMXerrorfunc callback, TMXuserptr user);

/**
 * @brief Retrieves a generic error message suitable for the given error type.
 * @param errno An error code indicating the general type of error that occurred.
 * @return The error string. This string must @b not be freed by the caller.
 */
const char *tmxErrorString(TMXenum errno);

/**
 * @brief Retrieves the first error (if any) that occurred since the last call to this function, then
 * resets the error state.
 *
 * @return The stored error state, or @ref TMX_ERR_NONE if no error has occurred.
 */
TMXenum tmxGetError(void);























/**
 * @brief Retrieves a property by its name.
 *
 * @param[in] properties The properties instance to query.
 * @param[in] name The name of the property to retrieve.
 * @param[out] property A pointer that will be assigned the property value, or @c NULL if just testing for the presence of
 * the property.
 *
 * @return @ref TMX_TRUE if property was found, otherwise @ref TMX_FALSE. When true, @a property will contain
 * the value, otherwise it will be assigned @c NULL.
 */
TMXbool tmxTryGetProperty(const TMXproperties *properties, const char *name, TMXproperty **property);

/**
 * @brief Retrieves a property by its name.
 *
 * @param[in] properties The properties instance to query.
 * @param[in] name The name of the property to retrieve.
 *
 * @return The property with the specified @a name, or @c NULL if none was found.
 */
TMXproperty *tmxGetProperty(const TMXproperties *properties, const char *name);

/**
 * @brief Retrieves the number of property objects stored in the hash.
 *
 * @param[in] properties The properties instance to query.
 * @return The number of property objects in the hash.
 */
size_t tmxGetPropertyCount(const TMXproperties *properties);

/**
 * @brief Retrieves the first property in the properties hash.
 *
 * @param[in] properties The properties instance to query.
 * @return The first property value, or @c NULL if empty.
 */
TMXproperty *tmxGetPropertiesHead(const TMXproperties *properties);

/**
 * @brief Helper macro to iterate properties.
 *
 * @param[in] properties The properties value to enumerate.
 * @param[in,out] property A pointer that will be assigned the property value within the loop.
 */
#define tmxPropertiesForeach(properties, property)                                                                                         \
    for ((property) = tmxGetPropertiesHead(properties); (property); (property) = (property)->next)



















/**
 * @brief Assigns a callback that can be invoked to provide user-loading of images as they are parsed from a document.
 *
 * @details This can be used configure an automatic load/free process for map resources, such as creating
 * required textures to render a map. The value returned by the callback is stored within the image structure
 * for convenient access to it while using the API.
 *
 * @param[in] load A callback that will be invoked when a TMX image is parsed from the document.
 * @param[in] free A callback that will be invoked when the TMX image is being freed to perform any necessary cleanup of the user-data.
 * @param[in] user An arbitrary user pointer that will be passed to the callbacks when invoked. Will never be modified by this library.
 */
TMX_PUBLIC void tmxImageCallback(TMXimageloadfunc load, TMXimagefreefunc free, TMXuserptr user);





TMX_PUBLIC void tmxTileForeach(TMXmap *map, TMXlayer *layer, TMXbool includeEmpty, TMXforeachfunc foreachFunc);









/**
 * @brief Frees a previously created map and all of its child objects.
 *
 * @param[in] map The map pointer to free.
 * @note Cached child objects will not be freed.
 */
TMX_PUBLIC void tmxFreeMap(TMXmap *map);

/**
 * @brief Frees a previously created tileset.
 *
 * @param[in] tileset The tileset pointer to free.
 * @note Cached child objects will not be freed.
 * @warning This function should only ever be called on a pointer that was explicitly loaded/parsed by the caller.
 * @exception If the tileset is cached, a @ref TMX_ERR_INVALID_OPERATION error will be emitted.
 */
TMX_PUBLIC void tmxFreeTileset(TMXtileset *tileset);

/**
 * @brief Frees a previously created template.
 *
 * @param[in] template The template pointer to free.
 * @note Cached child objects will not be freed.
 * @warning This function should only ever be called on a pointer that was explicitly loaded/parsed by the caller.
 * @exception If the template is cached, a @ref TMX_ERR_INVALID_OPERATION error will be emitted.
 */
TMX_PUBLIC void tmxFreeTemplate(TMXtemplate *template);

/**
 * @brief Loads a TMX map document from the specified path.
 * 
 * @param[in] filename The filesystem path containing the map definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 * 
 * @return The map object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXmap *tmxLoadMap(const char *filename, TMXcache *cache, TMXenum format);

/**
 * @brief Loads a TMX map document from the specifed text buffer.
 * 
 * @param[in] text A buffer containing the text contents of the map definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 * 
 * @return The map object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXmap *tmxParseMap(const char *text, TMXcache *cache, TMXenum format);

/**
 * @brief Loads a TMX tileset document from the specified path.
 * 
 * @param[in] filename The filesystem path containing the tileset definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 * 
 * @return The tileset object, or @c NULL if an error occurred. May return a cached instance when @a cache is supplied.
 */
TMX_PUBLIC TMXtileset *tmxLoadTileset(const char *filename, TMXcache *cache, TMXenum format);

/**
 * @brief Loads a TMX tileset document from the specifed text buffer.
 * 
 * @param[in] text A buffer containing the text contents of the tileset definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 * 
 * @return The tileset object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXtileset *tmxParseTileset(const char *text, TMXcache *cache, TMXenum format);

/**
 * @brief Loads a TMX template document from the specified path.
 * 
 * @param[in] filename The filesystem path containing the template definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 * 
 * @return The template object, or @c NULL if an error occurred. May return a cached instance when @a cache is supplied.
 */
TMX_PUBLIC TMXtemplate *tmxLoadTemplate(const char *filename, TMXcache *cache, TMXenum format);

/**
 * @brief Loads a TMX template document from the specifed text buffer.
 * 
 * @param[in] text A buffer containing the text contents of the template definition.
 * @param[in] cache An optional cache object to store reusable/shared map components.
 * @param[in] format Flag indicating the format of the document.
 * 
 * @return The template object, or @c NULL if an error occurred.
 */
TMX_PUBLIC TMXtemplate *tmxParseTemplate(const char *text, TMXcache *cache, TMXenum format);



#define tmxUserPtr(ptr) ((TMXuserptr){ptr})
#define tmxNullUserPtr  tmxUserPtr(NULL)


#ifndef TMX_DEBUG

/**
 * @brief Prints allocation/deallocation details to the standard output.
 */
void tmxMemoryLeakCheck(void);
#endif

#endif /* TMX_H */