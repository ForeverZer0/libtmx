#include "cwalk.h"
#include "internal.h"
#include "tmx/cache.h"
#include "tmx/compression.h"
#include "tmx/error.h"
#include "tmx/memory.h"
#include "tmx/types.h"
#include "tmx/xml.h"
#include <errno.h>
#include <stdio.h>

void tmxFreeTileset(TMXtileset *tileset); // TODO
void tmxFreeTemplate(TMXtemplate *template);

#pragma region Image

static TMXimageloadfunc imageLoad;
static TMXimagefreefunc imageFree;
static TMXuserptr imageUserPtr;

static TMXpathfunc pathResolve;
static TMXuserptr pathUserPtr;

void
tmxSetImageCallbacks(TMXimageloadfunc loadFunc, TMXimagefreefunc freeFunc, TMXuserptr user)
{
    imageLoad    = loadFunc;
    imageFree    = freeFunc;
    imageUserPtr = user;
}

#pragma endregion

#pragma region Cache

struct TMXentry
{
    const char *key;
    void *value;
    UT_hash_handle hh;
};

struct TMXcache
{
    TMXflag flags;
    struct TMXentry *tilesets;
    struct TMXentry *templates;
    struct TMXentry *images;
};

TMXbool
tmxCacheTryGet(TMXcache *cache, const char *key, void **result, TMXflag target)
{
    if (!cache || !key || !*result || target == TMX_CACHE_NONE)
        return TMX_FALSE;

    size_t len = strlen(key);
    if (!len || !TMX_FLAG(cache->flags, target))
        return TMX_FALSE;

    struct TMXentry **head, *entry = NULL;
    switch (target)
    {
        case TMX_CACHE_TILESET: head = &cache->tilesets; break;
        case TMX_CACHE_TEMPLATE: head = &cache->templates; break;
        case TMX_CACHE_IMAGE: head = &cache->images; break;
        default:
        {
            tmxError(TMX_ERR_PARAM);
            return TMX_FALSE;
        }
    }

    HASH_FIND(hh, *head, key, len, entry);
    if (entry)
    {
        *result = entry->value;
        return TMX_TRUE;
    }
    return TMX_FALSE;
}

// static struct TMXentry *tmxCacheAddImpl(struct TMXentry *headTEMP, const char *key, size_t keyLen, void *value)
// {
//     if (!head)
//         head = NULL;

//     struct TMXentry *head = head ? headTEMP : NULL;

//     struct TMXentry *entry;
//     entry        = tmxCalloc(1, sizeof(struct TMXentry));
//     entry->key   = tmxStringCopy(key, keyLen);
//     entry->value = value;
//     HASH_ADD_KEYPTR(hh, head, entry->key, keyLen, entry);

//     return head;
// }

TMXbool
tmxCacheAdd(TMXcache *cache, const char *key, void *value, TMXflag target)
{
    if (!cache || !key || !value || target == TMX_CACHE_NONE)
        return TMX_FALSE;

    size_t len = strlen(key);
    if (!len || !TMX_FLAG(cache->flags, target))
        return TMX_FALSE;

    struct TMXentry **head, *entry;
    switch (target)
    {
        case TMX_CACHE_TILESET:
            head = &cache->tilesets;
            ((TMXtileset *) value)->flags |= TMX_FLAG_CACHED;
            break;
        case TMX_CACHE_TEMPLATE:
            head = &cache->templates;
            ((TMXtemplate *) value)->flags |= TMX_FLAG_CACHED;
            break;
        case TMX_CACHE_IMAGE:
            head = &cache->images;
            ((TMXimage *) value)->flags |= TMX_FLAG_CACHED;
            break;
        default:
        {
            tmxError(TMX_ERR_PARAM);
            return TMX_FALSE;
        }
    }

    entry        = tmxCalloc(1, sizeof(struct TMXentry));
    entry->key   = tmxStringCopy(key, len);
    entry->value = value;
    HASH_ADD_KEYPTR(hh, *head, entry->key, len, entry);
    return TMX_TRUE;


    // switch (target)
    // {
    //     case TMX_CACHE_TILESET:
    //         cache->tilesets = tmxCacheAddImpl(cache->tilesets, key, len, value);
    //         break;
    //     case TMX_CACHE_TEMPLATE:
    //         cache->templates = tmxCacheAddImpl(cache->templates, key, len, value);
    //         break;
    //     case TMX_CACHE_IMAGE:
    //         cache->images = tmxCacheAddImpl(cache->images, key, len, value);
    //         break;
    //     default:
    //         tmxError(TMX_ERR_PARAM);
    //         return TMX_FALSE;
    // }

    return TMX_TRUE;
}

TMXbool
tmxCacheRemove(TMXcache *cache, const char *key, TMXflag target)
{
    if (!cache || !key || target == TMX_CACHE_NONE)
        return TMX_FALSE;

    size_t len = strlen(key);
    if (!len || !TMX_FLAG(cache->flags, target))
        return TMX_FALSE;

    struct TMXentry **head, *entry = NULL;
    switch (target)
    {
        case TMX_CACHE_TILESET: head = &cache->tilesets; break;
        case TMX_CACHE_TEMPLATE: head = &cache->templates; break;
        case TMX_CACHE_IMAGE: head = &cache->images; break;
        default:
        {
            tmxError(TMX_ERR_PARAM);
            return TMX_FALSE;
        }
    }

    HASH_FIND(hh, *head, key, len, entry);
    if (entry)
    {
        HASH_DEL(*head, entry);
        switch (target)
        {
            case TMX_CACHE_TILESET: ((TMXtileset *) entry->value)->flags &= ~(TMX_FLAG_CACHED); break;
            case TMX_CACHE_TEMPLATE: ((TMXtemplate *) entry->value)->flags &= ~(TMX_FLAG_CACHED); break;
            case TMX_CACHE_IMAGE: ((TMXimage *) entry->value)->flags &= ~(TMX_FLAG_CACHED); break;
        }
        return TMX_TRUE;
    }
    return TMX_FALSE;
}

size_t
tmxCacheClear(TMXcache *cache, TMXflag targets)
{
    if (!cache || targets == TMX_CACHE_NONE)
        return 0;

    size_t count = 0;
    struct TMXentry *entry, *temp;

    if (cache->tilesets && TMX_FLAG(targets, TMX_CACHE_TILESET))
    {
        TMXtileset *tileset;
        HASH_ITER(hh, cache->tilesets, entry, temp)
        {
            HASH_DEL(cache->tilesets, entry);
            tileset = (TMXtileset *) entry->value;
            tileset->flags &= ~TMX_FLAG_CACHED;
            tmxFreeTileset(tileset);
            tmxFree((void *) entry->key);
            tmxFree(entry);
            count++;
        }
    }

    if (cache->templates && TMX_FLAG(targets, TMX_CACHE_TEMPLATE))
    {
        TMXtemplate *template;
        HASH_ITER(hh, cache->templates, entry, temp)
        {
            HASH_DEL(cache->templates, entry);
            template = (TMXtemplate *) entry->value;
            template->flags &= ~TMX_FLAG_CACHED;
            tmxFreeTemplate(template);
            tmxFree((void *) entry->key);
            tmxFree(entry);
            count++;
        }
    }

    if (cache->images && TMX_FLAG(targets, TMX_CACHE_IMAGE))
    {
        TMXimage *image;
        HASH_ITER(hh, cache->images, entry, temp)
        {
            HASH_DEL(cache->images, entry);
            image = (TMXimage *) entry->value;
            image->flags &= ~TMX_FLAG_CACHED;
            tmxFreeImage(image);
            tmxFree((void *) entry->key);
            tmxFree(entry);
            count++;
        }
    }

    return count;
}

size_t
tmxCacheCount(TMXcache *cache, TMXflag targets)
{
    if (!cache)
        return 0;

    size_t count = 0;

    if (cache->tilesets && TMX_FLAG(targets, TMX_CACHE_TILESET))
        count += HASH_COUNT(cache->tilesets);

    if (cache->templates && TMX_FLAG(targets, TMX_CACHE_TEMPLATE))
        count += HASH_COUNT(cache->templates);

    if (cache->images && TMX_FLAG(targets, TMX_CACHE_IMAGE))
        count += HASH_COUNT(cache->images);

    return count;
}

TMXcache *
tmxCacheCreate(TMXflag targets)
{
    TMXcache *cache = tmxCalloc(1, sizeof(TMXcache));
    cache->flags    = targets;
    return cache;
}

void
tmxFreeCache(TMXcache *cache)
{
    if (!cache)
        return;
    tmxCacheClear(cache, TMX_CACHE_ALL);
    tmxFree(cache);
}

#pragma endregion

TMX_COLOR_T
tmxParseColor(const char *str)
{
    // clang-format off
    // Early out to zero color if string is empty.
    TMX_COLOR_T color = {0};
    if (!str)
        return color;

    // Skip the prefix
    if (*str == '#') { str++; }

    // Calculate the value as an unsigned integer, accounting for different formats/lengths
	size_t len = strlen(str);
	uint32_t u32 = (uint32_t) strtoul(str, NULL, 16);
	if (len < 6) {
		u32 = (u32 & 0xF000u) << 16 | (u32 & 0xF000u) << 12
		    | (u32 & 0x0F00u) << 12 | (u32 & 0x0F00u) <<  8
		    | (u32 & 0x00F0u) <<  8 | (u32 & 0x00F0u) <<  4
		    | (u32 & 0x000Fu) <<  4 | (u32 & 0x000Fu);     
	}
	if (len == 6 || len == 3)
        u32 |= 0xFF000000u;

    // If using packed colors, just assign the value, else normalize it.
    #ifdef TMX_PACKED_COLOR
    a.value = u32;
    #else
    color.a = (unsigned char) ((u32 >> 24) & 0xFF);
    color.r = (unsigned char) ((u32 >> 16) & 0xFF);
    color.g = (unsigned char) ((u32 >>  8) & 0xFF);
    color.b = (unsigned char) ((u32 >>  0) & 0xFF);
    #endif

    // clang-format on

    return color;
}

TMX_INLINE char *
tmxStringCopy(const char *input, size_t inputSize)
{
    if (!input)
        return NULL;

    size_t len   = inputSize ? inputSize : strlen(input);
    char *result = tmxMalloc(len + 1);
    memcpy(result, input, len);
    result[len] = '\0';
    return result;
}

inline TMXbool
tmxStringBool(const char *str)
{
    if (str[0] == '0')
        return TMX_FALSE;
    if (str[0] == '1')
        return TMX_TRUE;
    if (STREQL(str, "true"))
        return TMX_TRUE;
    if (STREQL(str, "false"))
        return TMX_FALSE;

    return TMX_FALSE;
}

char *
tmxFileReadAll(const char *filename, size_t *size)
{
    FILE *fp     = NULL;
    char *buffer = NULL;
    size_t len;

    if (!filename)
    {
        tmxError(TMX_ERR_VALUE);
        goto FAIL;
    }

    fp = fopen(filename, "r");
    if (!fp)
    {
        tmxErrorFormat(TMX_ERR_IO, "Could not open file \"%s\".", filename);
        goto FAIL;
    }

    fseek(fp, 0, SEEK_END);
    len = (size_t) ftell(fp);
    if (!len)
        goto FAIL;
    fseek(fp, 0, SEEK_SET);

    buffer = tmxMalloc(len + 1);
    if (!buffer)
        goto FAIL;

    if (fread(buffer, 1, len, fp) != len)
    {
        tmxErrorFormat(TMX_ERR_IO, "Failed to read from \"%s\".", filename);
        goto FAIL;
    }

    buffer[len] = '\0';
    if (size)
        *size = len;
    return buffer;

FAIL:
    if (size)
        *size = 0;
    if (buffer)
        tmxFree(buffer);
    if (fp)
        fclose(fp);
    return NULL;
}

static inline TMXbool
tmxFileTest(const char *path)
{
    FILE *fp = NULL;
    fp       = fopen(path, "r");
    if (fp)
    {
        fclose(fp);
        return TMX_TRUE;
    }
    return TMX_FALSE;
}

size_t
tmxPathResolve(const char *path, const char *baseDir, char *buffer, size_t bufferSize)
{
    size_t len;

    // Use built-in method to find path if base-directory was supplied.
    if (baseDir)
    {
        len                              = cwk_path_get_absolute(baseDir, path, buffer, bufferSize);
        buffer[TMX_MIN(len, bufferSize)] = '\0';
        if (len && tmxFileTest(buffer))
            return len;
        len = 0;
    }

    // Test relative path and return if found.
    if (tmxFileTest(path))
    {
        len = strlen(path);
        len = TMX_MIN(len, bufferSize - 1);
        memcpy(buffer, path, len);
        buffer[len] = '\0';
        return len;
    }

    // Couldn't find it, so invoke the user-callback to resolve it.
    if (pathResolve)
        len = pathResolve(path, baseDir, buffer, bufferSize, pathUserPtr);

    // If still not found, emit an error.
    if (!len)
    {
        if (errno == ENOENT)
            tmxErrorFormat(TMX_ERR_IO, "No such file or directory: \"%s\".", path);
        else if (errno == EACCES)
            tmxErrorFormat(TMX_ERR_IO, "Permission denied accessing \"%s\".", path);
    }

    return len;
}

void
tmxImageUserLoad(TMXimage *image, const char *baseDir, TMXcache *cache)
{
    if (!imageLoad)
        return;

    imageLoad(image, baseDir, imageUserPtr);
    tmxCacheAddImage(cache, image->source, image);
}

void
tmxImageUserFree(TMXimage *image)
{
    if (imageFree)
        imageFree(image->user_data, imageUserPtr);
}

void
tmxSetPathResolveCallback(TMXpathfunc callback, TMXuserptr user)
{
    pathResolve = callback;
    pathUserPtr = user;
}