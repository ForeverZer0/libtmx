#include "TMX/cache.h"
#include "TMX/memory.h"
#include "TMX/typedefs.h"
#include "utils.h"

#define uthash_malloc(sz)    tmxMalloc(sz)
#define uthash_free(ptr, sz) tmxFree(ptr)
#include "uthash.h"

struct TMXkeyvalue
{
    const char *key;
    void *value;
    UT_hash_handle hh;
};

struct TMXcache
{
    struct TMXkeyvalue tilesets;
    struct TMXkeyvalue templates;
    struct TMXkeyvalue images;
};

TMXbool
tmxCacheGet(TMXcache *cache, const char *key, void **result, TMXflag target)
{
    size_t len = strlen(key);
    if (!cache || !key || !len || !*result)
        return TMX_FALSE;

    struct TMXkeyvalue *head, *entry;
    switch (target)
    {
        case TMX_CACHE_TILESET: head = &cache->tilesets; break;
        case TMX_CACHE_TEMPLATE: head = &cache->templates; break;
        case TMX_CACHE_IMAGE: head = &cache->images; break;
        default: return TMX_FALSE;
    }

    HASH_FIND(hh, head, key, len, entry);
    if (entry)
    {
        *result = entry->value;
        return TMX_TRUE;
    }
    return TMX_FALSE;
}

TMXbool
tmxCacheAdd(TMXcache *cache, const char *key, void *value, TMXflag target)
{
    size_t len = strlen(key);
    if (!key || !len || !value)
        return TMX_FALSE;

    struct TMXkeyvalue *head, *entry;
    switch (target)
    {
        case TMX_CACHE_TILESET: head = &cache->tilesets; break;
        case TMX_CACHE_TEMPLATE: head = &cache->templates; break;
        case TMX_CACHE_IMAGE: head = &cache->images; break;
        default: return TMX_FALSE;
    }

    entry        = tmxCalloc(1, sizeof(struct TMXkeyvalue));
    entry->key   = tmxStringCopy(key, len);
    entry->value = value;
    HASH_ADD_KEYPTR(hh, head, entry->key, len, entry);
    return TMX_TRUE;
}

size_t
tmxCacheClear(TMXcache *cache, TMXflag targets)
{
    if (!cache || targets == TMX_CACHE_NONE)
        return;

    size_t count = 0;
    struct TMXkeyvalue *head, *entry, *temp;

    if (TMX_FLAG(targets, TMX_CACHE_TILESET))
    {
        head = &cache->tilesets;
        HASH_ITER(hh, head, entry, temp)
        {
            HASH_DEL(head, entry);
            tmxFree(entry->key);
            // TODO: Call free function
            count++;
        }
    }

    if (TMX_FLAG(targets, TMX_CACHE_TEMPLATE))
    {
        head = &cache->templates;
        HASH_ITER(hh, head, entry, temp)
        {
            HASH_DEL(head, entry);
            tmxFree(entry->key);
            // TODO: Call free function
            count++;
        }
    }

    if (TMX_FLAG(targets, TMX_CACHE_IMAGE))
    {
        head = &cache->images;
        HASH_ITER(hh, head, entry, temp)
        {
            HASH_DEL(head, entry);
            tmxFree(entry->key);
            // TODO: Call free function
            count++;
        }
    }

    return count;
}

size_t
tmxCacheCount(TMXcache *cache, TMXflag targets)
{
    if (!cache || targets == TMX_CACHE_NONE)
        return 0;

    size_t count = 0;

    if (TMX_FLAG(targets, TMX_CACHE_TILESET))
        count += HASH_COUNT(&cache->tilesets);

    if (TMX_FLAG(targets, TMX_CACHE_TEMPLATE))
        count += HASH_COUNT(&cache->templates);

    if (TMX_FLAG(targets, TMX_CACHE_IMAGE))
        count += HASH_COUNT(&cache->images);

    return count;
}

TMXcache *
tmxCacheCreate(void)
{
    return tmxCalloc(1, sizeof(TMXcache));
}

void
tmxFreeCache(TMXcache *cache)
{
    if (!cache)
        return;
    tmxCacheClear(cache, TMX_CACHE_ALL);
    tmxFree(cache);
}
