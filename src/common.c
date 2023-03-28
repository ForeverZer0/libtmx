#include "internal.h"
#include "tmx/compression.h"
#include "tmx/memory.h"
#include "tmx/xml.h"
#include <errno.h>
#include <stdio.h>

#pragma region Image

static TMXimageloadfunc imageLoad;
static TMXimagefreefunc imageFree;
static TMXuserptr imageUserPtr;

TMXcolor tmxColor(const TMXcolorf *color)
{
    TMXcolor packed = {0};
    if (!color)
        return packed;

    packed.r = (uint8_t) (color->r * 255.0f);
    packed.g = (uint8_t) (color->g * 255.0f);
    packed.b = (uint8_t) (color->b * 255.0f);
    packed.a = (uint8_t) (color->a * 255.0f);
    return packed;
}

TMXcolorf tmxColorF(TMXcolor color)
{
    TMXcolorf vector;
    vector.r = ((float) color.r / 255.0f);
    vector.g = ((float) color.g / 255.0f);
    vector.b = ((float) color.b / 255.0f);
    vector.a = ((float) color.a / 255.0f);
    return vector;
}

void
tmxImageCallback(TMXimageloadfunc loadFunc, TMXimagefreefunc freeFunc, TMXuserptr user)
{
    imageLoad    = loadFunc;
    imageFree    = freeFunc;
    imageUserPtr = user;
}

void
tmxImageUserLoad(TMXimage *image, const char *basePath)
{
    if (!imageLoad)
        return;
    image->user_data = imageLoad(image, basePath, imageUserPtr);
}

void
tmxImageUserFree(TMXimage *image)
{
    if (imageFree)
        imageFree(image->user_data, imageUserPtr);
}

#pragma endregion

#pragma region Cache

struct TMXentry
{
    void *value;
    UT_hash_handle hh;
};

struct TMXcache
{
    TMXflag flags;
    struct TMXentry *tilesets;
    struct TMXentry *templates;
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

TMXbool
tmxCacheAdd(TMXcache *cache, const char *key, void *value, TMXflag target)
{
    if (!cache || !key || !value || target == TMX_CACHE_NONE || !TMX_FLAG(cache->flags, target))
        return TMX_FALSE;

    size_t len = strlen(key);
    if (!len)
        return TMX_FALSE;

    struct TMXentry *entry = TMX_ALLOC(struct TMXentry);
    entry->value           = value;

    switch (target)
    {
        case TMX_CACHE_TILESET:
            HASH_ADD_KEYPTR(hh, cache->tilesets, key, len, entry);
            ((TMXtileset *) value)->flags |= TMX_FLAG_CACHED;
            break;
        case TMX_CACHE_TEMPLATE:
            HASH_ADD_KEYPTR(hh, cache->templates, key, len, entry);
            ((TMXtemplate *) value)->flags |= TMX_FLAG_CACHED;
            break;
        default:
        {
            tmxFree(entry);
            tmxError(TMX_ERR_PARAM);
            return TMX_FALSE;
        }
    }

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

static TMX_INLINE TMXtile *tmxGetTile(TMXmap *map, TMXgid gid)
{
    for (size_t i = 0; i < map->tileset_count; i++)
    {
        // TODO: Tilesets are ordered by first GID, but maybe still check range Just-In-Case™ 
        // What if someone sorts the list to suit their own needs?
        if (gid >= map->tilesets[i].first_gid)
            return &map->tilesets[i].tileset->tiles[gid - map->tilesets[i].first_gid];
    }
    return NULL;
}

void
tmxTileForeach(TMXmap *map, TMXlayer *layer, TMXbool includeEmpty, TMXforeachfunc foreachFunc)
{
    if (!map || !layer || !foreachFunc || layer->type != TMX_LAYER_TILE)
    {
        tmxError(TMX_ERR_VALUE);
        return;
    }

    int i, count = (int) layer->count;
    int width = (int) layer->size.w;
    int height = (int) layer->size.h;
    TMXtile *tile = NULL;
    TMXgid gid = 0;
 
    switch (map->render_order)
    {
        case TMX_RENDER_RIGHT_DOWN: 
            for (i = 0; i < count; i++)
            {
                gid = layer->data.tiles[i];
                tile = tmxGetTile(map, gid & TMX_GID_TILE_MASK);
                if (tile || includeEmpty)
                {
                    if (!foreachFunc(map, layer, tile, i % width, i / width, gid))
                        break;
                }
            }
            break;
        case TMX_RENDER_RIGHT_UP: 
            for (i = 0; i < count; i++)
            {
                gid = layer->data.tiles[i];
                tile = tmxGetTile(map, gid & TMX_GID_TILE_MASK);
                if (tile || includeEmpty)
                {
                    if (!foreachFunc(map, layer, tile, i % width, height - (i / width), gid))
                        break;
                }
            }
            break;
        case TMX_RENDER_LEFT_DOWN:
            for (i = 0; i < count; i++)
            {
                gid = layer->data.tiles[i];
                tile = tmxGetTile(map, gid & TMX_GID_TILE_MASK);
                if (tile || includeEmpty)
                {
                    if (!foreachFunc(map, layer, tile, width - (i % width), i / width, gid))
                        break;
                }
            }
            break;
        case TMX_RENDER_LEFT_UP: 
            for (i = 0; i < count; i++)
            {
                gid = layer->data.tiles[i];
                tile = tmxGetTile(map, gid & TMX_GID_TILE_MASK);
                if (tile || includeEmpty)
                {
                    if (!foreachFunc(map, layer, tile, width - (i % width), height - (i / width), gid))
                        break;
                }
            }
            break;
        default: tmxError(TMX_ERR_PARAM); break;
    }
}

void
tmxObjectMergeTemplate(TMXobject *dst, TMXobject *src)
{
    // Because the XML parser is forward-only reading, we can't first check for a template, copy it, and then only
    // modify the duplicated object according to what it overrides. Instead, the object is defined normally
    // and then each field checked. If the field was not explicitly defined in the object, it is assigned the
    // value from its template. 

    dst->type = src->type;

    if (!TMX_FLAG(dst->flags, TMX_FLAG_NAME) && src->name)
        dst->name = tmxStringDup(src->name);
    if (!TMX_FLAG(dst->flags, TMX_FLAG_CLASS) && src->class)
        dst->class = tmxStringDup(src->class);
    if (!TMX_FLAG(dst->flags, TMX_FLAG_GID))
        dst->gid = src->gid;
    if (!TMX_FLAG(dst->flags, TMX_FLAG_POSITION))
        dst->position = src->position;
    if (!TMX_FLAG(dst->flags, TMX_FLAG_SIZE))
        dst->size = src->size;
    if (!TMX_FLAG(dst->flags, TMX_FLAG_ROTATION))
        dst->rotation = src->rotation;
    if (!TMX_FLAG(dst->flags, TMX_FLAG_VISIBLE))
        dst->visible = src->visible;

    // Text-specific fields
    if (dst->type == TMX_OBJECT_TEXT)
    {
        if (!dst->text)
            dst->text = TMX_ALLOC(struct TMXtext);

        if (!dst->text->string && !TMX_FLAG(dst->flags, TMX_FLAG_TEXT) && src->text->string)
            dst->text->string = tmxStringDup(src->text->string);

        if (!dst->text->font && !TMX_FLAG(dst->flags, TMX_FLAG_FONT) && src->text->font)
            dst->text->font = tmxStringDup(src->text->font);

        if (!TMX_FLAG(dst->flags, TMX_FLAG_FONT_BOLD) && TMX_FLAG(src->text->style, TMX_FONT_STYLE_BOLD))
            dst->text->style |= TMX_FONT_STYLE_BOLD;

        if (!TMX_FLAG(dst->flags, TMX_FLAG_FONT_ITALIC) && TMX_FLAG(src->text->style, TMX_FONT_STYLE_ITALIC))
            dst->text->style |= TMX_FONT_STYLE_ITALIC;

        if (!TMX_FLAG(dst->flags, TMX_FLAG_FONT_UNDERLINE) && TMX_FLAG(src->text->style, TMX_FONT_STYLE_UNDERLINE))
            dst->text->style |= TMX_FONT_STYLE_UNDERLINE;

        if (!TMX_FLAG(dst->flags, TMX_FLAG_FONT_STRIKEOUT) && TMX_FLAG(src->text->style, TMX_FONT_STYLE_STRIKEOUT))
            dst->text->style |= TMX_FONT_STYLE_STRIKEOUT;

        if (!TMX_FLAG(dst->flags, TMX_FLAG_HALIGN))
        {
            if (TMX_FLAG(src->flags, TMX_FLAG_HALIGN))
                dst->text->align |= (src->text->align & ~TMX_ALIGN_CENTER_V);
            else
                dst->text->align |= TMX_ALIGN_LEFT;
        }
        
        if (!TMX_FLAG(dst->flags, TMX_FLAG_VALIGN))
        {
            if (TMX_FLAG(src->flags, TMX_FLAG_VALIGN))
                dst->text->align |= (src->text->align & ~TMX_ALIGN_CENTER_H);
            else
                dst->text->align |= TMX_ALIGN_TOP;
        }

        if (!TMX_FLAG(dst->flags, TMX_FLAG_FONT_SIZE))
            dst->text->pixel_size = src->text->pixel_size;

        if (!TMX_FLAG(dst->flags, TMX_FLAG_FONT_KERNING))
            dst->text->kerning = src->text->kerning;

        if (!TMX_FLAG(dst->flags, TMX_FLAG_WORD_WRAP))
            dst->text->wrap = src->text->wrap;
    }

    // Polygon/polyline specific fields.
    else if ((dst->type == TMX_OBJECT_POLYGON || dst->type == TMX_OBJECT_POLYLINE) && !dst->poly.points)
    {
        dst->poly.count  = src->poly.count;
        size_t pntSize   = dst->poly.count * sizeof(TMXvec2);
        dst->poly.points = tmxMalloc(pntSize);
        memcpy(dst->poly.points, src->poly.points, pntSize);
    }

    // Finally, merge/copy properties
    dst->properties = tmxPropertiesMerge(dst->properties, src->properties);
}