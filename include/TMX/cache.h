#ifndef TMX_CACHE_H
#define TMX_CACHE_H

#include "typedefs.h"
#include <stddef.h>

TMXbool tmxCacheTryGetTileset(TMXcache *cache, const char *key, TMXtileset **tileset);

TMXbool tmxCacheTryGetTemplate(TMXcache *cache, const char *key, TMXtemplate **template);

TMXbool tmxCacheAddTileset(TMXcache *cache, const char *key, TMXtileset *tileset);

TMXbool tmxCacheAddTemplate(TMXcache *cache, const char *key, TMXtemplate *template);

void tmxCacheClear(TMXcache *cache);

void tmxCacheCount(TMXcache *cache, size_t *numTilesets, size_t *numTemplates);

TMXcache *tmxCacheCreate(void);

void tmxFreeCache(TMXcache *cache);

#endif /* TMX_CACHE_H */