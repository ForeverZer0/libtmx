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

#include "tmx/types.h"

#define TMX_GID_CLEAN(x) ((x) &TMX_GID_MASK)
#define TMX_GID_FLAGS(x) ((x) & ~TMX_GID_MASK)

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
 */
TMX_PUBLIC void tmxFreeTileset(TMXtileset *tileset);

/**
 * @brief Frees a previously created template.
 *
 * @param[in] template The template pointer to free.
 * @note Cached child objects will not be freed.
 * @warning This function should only ever be called on a pointer that was explicitly loaded/parsed by the caller.
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

#endif /* TMX_H */