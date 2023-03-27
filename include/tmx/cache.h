#ifndef TMX_CACHE_H
#define TMX_CACHE_H

#include "types.h"

/**
 * @defgroup cache Cache
 * @brief Provides mechanisms for caching commonly used objects and/or objects that are
 * shared by multiple components. This allows them to be loaded/parsed once, and reused
 * as-needed.
 */

/**
 * @defgroup cachetypes Cache Targets
 * @brief Bit-flags describing supported TMX types that can be cached.
 *
 * @ingroup cache
 * @{
 */

#define TMX_CACHE_NONE     0x00 /** None/invalid target. */
#define TMX_CACHE_TEMPLATE 0x01 /** Object templates. */
#define TMX_CACHE_TILESET  0x02 /** Tilesets used by tile layers and objects. */
#define TMX_CACHE_ALL      0xFF /** Targets all supported cache types. */

/* @} */

/**
 * Helper macro to retrieve a tileset from the cache.
 * @param[in] cache The cache to query.
 * @param[in] key The key of the item to retrieve.
 * @param[out] result The address of a tileset pointer to store the result upon success.
 * @return @c TMX_TRUE when object was successfully retrieved, otherwise @c TMX_FALSE
 */
#define tmxCacheTryGetTileset(cache, key, result) tmxCacheTryGet((cache), (key), (void**)(result), TMX_CACHE_TILESET)

/**
 * Helper macro to retrieve a template from the cache.
 * @param[in] cache The cache to query.
 * @param[in] key The key of the item to retrieve.
 * @param[out] result The address of a template pointer to store the result upon success.
 * @return @c TMX_TRUE when object was successfully retrieved, otherwise @c TMX_FALSE
 */
#define tmxCacheTryGetTemplate(cache, key, result) tmxCacheTryGet((cache), (key), (void**)(result), TMX_CACHE_TEMPLATE)

/**
 * Helper macro to add a tileset to the cache.
 * @param[in] cache The cache to add an item to.
 * @param[in] key The key that will be used to store/retrieve the item.
 * @param[in] tileset A pointer to a tileset.
 * @return @c TMX_TRUE when object was successfully added, otherwise @c TMX_FALSE if insertion failed.
 */
#define tmxCacheAddTileset(cache, key, tileset) tmxCacheAdd(cache, key, tileset, TMX_CACHE_TILESET)

/**
 * Helper macro to add a template to the cache.
 * @param[in] cache The cache to add an item to.
 * @param[in] key The key that will be used to store/retrieve the item.
 * @param[in] template A pointer to a template.
 * @return @c TMX_TRUE when object was successfully added, otherwise @c TMX_FALSE if insertion failed.
 */
#define tmxCacheAddTemplate(cache, key, template) tmxCacheAdd(cache, key, template, TMX_CACHE_TEMPLATE)

/**
 * @brief Attempts to add an item of the specifed type to the cache. If the key already exists in the specified target, it will not be
 * inserted or overwrite it. To replace an item, it must first be removed with @ref tmxCacheRemove or @ref tmxCacheClear.
 *
 * @param[in] cache The cache to add an item to.
 * @param[in] key The key that will be used to store/retrieve the item.
 * @param[in] obj The object to add.
 * @param[in] target A single flag indicating which type of object this is.
 *
 * @return @c TMX_TRUE when object was successfully added, otherwise @c TMX_FALSE if insertion failed.
 */
TMXbool tmxCacheAdd(TMXcache *cache, const char *key, void *obj, TMXflag target);

/**
 * @brief Attempts to retrieve an item of the specified type and key from the cache.
 *
 * @param[in] cache The cache to query.
 * @param[in] key The key of the item to retrieve.
 * @param[out] result A pointer to store the result upon success.
 * @param[in] target A single flag indicating the type of object to retrieve.
 *
 * @return Indicates if an item was successfully found. When @c TMX_TRUE, the @a outResult will contain
 * a valid object, otherwise if @c TMX_FALSE it should not be used.
 */
TMXbool tmxCacheTryGet(TMXcache *cache, const char *key, void **result, TMXflag target);

/**
 * @brief Deletes an item of the specified type from the cache.
 *
 * @param[in] cache The cache to delete an item from.
 * @param[in] key The key of the value to remove.
 * @param[in] target A single flag indicating the type of object to retrieve.
 *
 * @return @c TMX_TRUE if item was successfully removed, otherwise @c TMX_FALSE.
 */
TMXbool tmxCacheRemove(TMXcache *cache, const char *key, TMXflag target);

/**
 * @brief Removes and frees the items in the cache of the specified type(s).
 *
 * @param[in] cache The cache to remove items from.
 * @param[in] targets A sets of bit-flags OR'ed together determining which type of item(s) to remove.
 *
 * @return The number of items successfully removed.
 */
size_t tmxCacheClear(TMXcache *cache, TMXflag targets);

/**
 * @brief Retrieves the number of items in the cache.
 * @param[in] cache The cache to query.
 * @param[in] targets A sets of bit-flags OR'ed together determining which type of item(s) to count.
 *
 * @return The number of items in the cache of the specified type(s).
 */
size_t tmxCacheCount(TMXcache *cache, TMXflag targets);

/**
 * @brief Initializes a new instance of a cache and returns it.
 * @param[in] targets A sets of bit-flags OR'ed together determining which types will be automatically added to the cache.
 * @return The newly created object, which must be freed with @ref tmxFreeCache.
 */
TMXcache *tmxCacheCreate(TMXflag targets);

/**
 * @brief Frees the cache and all of the items it contains.
 * @param[in] cache The cache to free.
 */
void tmxFreeCache(TMXcache *cache);

#endif /* TMX_CACHE_H */