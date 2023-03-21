#ifndef TMX_CACHE_H
#define TMX_CACHE_H

#include "typedefs.h"
#include <stddef.h>

#define TMX_CACHE_NONE     0x00
#define TMX_CACHE_TEMPLATE 0x01
#define TMX_CACHE_TILESET  0x02
#define TMX_CACHE_IMAGE    0x04
#define TMX_CACHE_ALL      0xFF
#define TMX_CACHE_DEFAULT  (TMX_CACHE_TEMPLATE | TMX_CACHE_TILESET)

#define tmxCacheTryGetTileset(cache, key, result)  tmxCacheTryGet(cache, key, result, TMX_CACHE_TILESETS)
#define tmxCacheTryGetTemplate(cache, key, result) tmxCacheTryGet(cache, key, result, TMX_CACHE_TEMPLATE)
#define tmxCacheTryGetImage(cache, key, result)    tmxCacheTryGet(cache, key, result, TMX_CACHE_IMAGE)

#define tmxCacheAddTileset(cache, key, tileset)   tmxCacheAdd(cache, key, tileset, TMX_CACHE_TILESET)
#define tmxCacheAddTemplate(cache, key, template) tmxCacheAdd(cache, key, template, TMX_CACHE_TEMPLATE)
#define tmxCacheAddImage(cache, key, image)       tmxCacheAdd(cache, key, image, TMX_CACHE_IMAGE)

TMXbool tmxCacheAdd(TMXcache *cache, const char *key, void *obj, TMXflag target);

TMXbool tmxCacheTryGet(TMXcache *cache, const char *key, void **outResult, TMXflag target);

size_t tmxCacheClear(TMXcache *cache, TMXflag targets);

size_t tmxCacheCount(TMXcache *cache, TMXflag targets);

TMXcache *tmxCacheCreate(void);

void tmxFreeCache(TMXcache *cache);

#endif /* TMX_CACHE_H */