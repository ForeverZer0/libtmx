#include "internal.h"
#include "tmx/cache.h"
#include "tmx/error.h"
#include "tmx/memory.h"
#include "tmx/compression.h"
#include "tmx/types.h"
#include "tmx/xml.h"

#include "uthash.h"


void tmxFreeTileset(TMXtileset *tileset); // TODO
void tmxFreeTemplate(TMXtemplate *template);


static void tmxXmlReadDataType(TMXxmlreader *xml, TMXenum *encoding, TMXenum *compression)
{

    *encoding = TMX_ENCODING_NONE;
    *compression = TMX_COMPRESSION_NONE;

    const char *name;
    const char *value;

    while (tmxXmlReadAttr(xml, &name, &value))
    {
        if (STREQL(name, "encoding"))
        {
            if (STREQL(value, "base64"))
                *encoding = TMX_ENCODING_BASE64;
            else if (STREQL(value, "csv"))
                *encoding = TMX_ENCODING_CSV;
        }
        else if (STREQL(name, "compression"))
        {
            if (STREQL(value, "gzip"))
                *compression = TMX_COMPRESSION_GZIP;
            else if (STREQL(value, "zlib"))
                *compression = TMX_COMPRESSION_ZLIB;
            else if (STREQL(value, "zstd"))
                *compression = TMX_COMPRESSION_ZSTD;
        }
    }
}

#pragma region Image

static TMXimageloadfunc imageLoad;
static TMXimagefreefunc imageFree;
static TMXuserptr imageUserPtr;

void
tmxSetImageCallbacks(TMXimageloadfunc loadFunc, TMXimagefreefunc freeFunc, TMXuserptr user)
{
    imageLoad    = loadFunc;
    imageFree    = freeFunc;
    imageUserPtr = user;
}

TMXimage *
tmxXmlReadImage(TMXxmlreader *xml, TMXcontext *context)
{
    const char *name;
    const char *value;
    size_t size;
    TMXimage *image = tmxCalloc(1, sizeof(TMXimage));

    while (tmxXmlReadAttr(xml, &name, &value))
    {
        if (STREQL(name, "format"))
            image->format = tmxStringDup(value);
        else if (STREQL(name, "source"))
        {
            image->source = tmxStringDup(value);
            image->flags |= TMX_FLAG_EXTERNAL;
        }
        else if (STREQL(name, "trans"))
        {
            image->transparent = tmxParseColor(value);
            image->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, "width"))
            image->size.w = TMX_STR2INT(value);
        else if (STREQL(name, "height"))
            image->size.h = TMX_STR2INT(value);
    }

    tmxXmlMoveToContent(xml);
    while (tmxXmlReadElement(xml, &name, &size))
    {
        if (!STREQL(name, "data"))
            continue;

        image->flags |= TMX_FLAG_EMBEDDED;

        TMXenum encoding, compression;
        tmxXmlReadDataType(xml, &encoding, &compression);
        tmxXmlMoveToContent(xml);

        if (compression != TMX_COMPRESSION_NONE)
        {
            tmxErrorMessage(TMX_ERR_UNSUPPORTED, "Compressed image data is not supported.");
            break;
        }

        const char *contents;
        if (tmxXmlReadStringContents(xml, &contents, &size, TMX_TRUE))
        {
            size_t dataSize = tmxBase64DecodedSize(contents, size);
            image->data = tmxMalloc(dataSize);
            if (!image->data)
                break;
            tmxBase64Decode(contents, size, image->data, dataSize);
        }
        break;
    }

    if (imageLoad)
        imageLoad(image, context->baseDir, imageUserPtr);

    return image;
}

void
tmxFreeImage(TMXimage *image)
{
    if (!image)
        return;

    if (imageFree)
        imageFree(image->user_data, imageUserPtr);

    tmxFree((void*)image->format);
    tmxFree(image->data);
    tmxFree((void*)image->source);
    tmxFree(image);
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
    struct TMXentry tilesets;
    struct TMXentry templates;
    struct TMXentry images;
};

TMXbool
tmxCacheGet(const TMXcache *cache, const char *key, void **result, TMXflag target)
{
    if (!cache || !key || !*result || target == TMX_CACHE_NONE)
        return TMX_FALSE;

    size_t len = strlen(key);
    if (!len || !TMX_FLAG(cache->flags, target))
        return TMX_FALSE;

    const struct TMXentry *head, *entry = NULL;
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
    if (!cache || !key || !value || target == TMX_CACHE_NONE)
        return TMX_FALSE;

    size_t len = strlen(key);
    if (!len || !TMX_FLAG(cache->flags, target))
        return TMX_FALSE;

    struct TMXentry *head, *entry;
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

    entry        = tmxCalloc(1, sizeof(struct TMXentry));
    entry->key   = tmxStringCopy(key, len);
    entry->value = value;
    HASH_ADD_KEYPTR(hh, head, entry->key, len, entry);
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

    struct TMXentry *head, *entry = NULL;
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

    HASH_FIND(hh, head, key, len, entry);
    if (entry)
    {
        HASH_DEL(head, entry);
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
    struct TMXentry *head, *entry, *temp;

    if (TMX_FLAG(cache->flags, TMX_CACHE_TILESET) && TMX_FLAG(targets, TMX_CACHE_TILESET))
    {
        head = &cache->tilesets;
        HASH_ITER(hh, head, entry, temp)
        {
            HASH_DEL(head, entry);
            tmxFree((void *) entry->key);
            // TODO: Call free function
            count++;
        }
    }

    if (TMX_FLAG(cache->flags, TMX_CACHE_TEMPLATE) && TMX_FLAG(targets, TMX_CACHE_TEMPLATE))
    {
        head = &cache->templates;
        HASH_ITER(hh, head, entry, temp)
        {
            HASH_DEL(head, entry);
            tmxFree((void *) entry->key);
            // TODO: Call free function
            count++;
        }
    }

    if (TMX_FLAG(cache->flags, TMX_CACHE_IMAGE) && TMX_FLAG(targets, TMX_CACHE_IMAGE))
    {
        head = &cache->images;
        HASH_ITER(hh, head, entry, temp)
        {
            HASH_DEL(head, entry);
            tmxFree((void *) entry->key);
            tmxFreeImage(entry->value);
            count++;
        }
    }

    return count;
}

size_t
tmxCacheCount(const TMXcache *cache, TMXflag targets)
{
    if (!cache)
        return 0;

    size_t count = 0;

    if (TMX_FLAG(cache->flags, TMX_CACHE_TILESET) && TMX_FLAG(targets, TMX_CACHE_TILESET))
        count += HASH_COUNT(&cache->tilesets);

    if (TMX_FLAG(cache->flags, TMX_CACHE_TEMPLATE) && TMX_FLAG(targets, TMX_CACHE_TEMPLATE))
        count += HASH_COUNT(&cache->templates);

    if (TMX_FLAG(cache->flags, TMX_CACHE_IMAGE) && TMX_FLAG(targets, TMX_CACHE_IMAGE))
        count += HASH_COUNT(&cache->images);

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

TMXbool
tmxStringBool(const char *str)
{
    return TMX_FALSE; // TODO
}