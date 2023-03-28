#include "internal.h"

#ifdef TMX_DEBUG
static size_t allocationCount;
static size_t deallocationCount;
#endif

#if !defined(TMX_REALLOC)

#if defined(TMX_MALLOC) || defined(TMX_FREE) || defined(TMX_CALLOC)
#error TMX_REALLOC must be defined in order to define any of TMX_MALLOC, TMX_CALLOC, or TMX_FREE
#endif

#include <stdlib.h>
#define TMX_MALLOC(ptr, user)                 malloc(ptr)
#define TMX_REALLOC(previous, newSize, user)  realloc(previous, newSize)
#define TMX_CALLOC(elemCount, elemSize, user) calloc(elemCount, elemSize)
#define TMX_FREE(ptr, user)                   free(ptr)

#else

#if !defined(TMX_FREE)
#define TMX_FREE(ptr, user) TMX_UNUSED(TMX_REALLOC(ptr, 0, user))
#endif

#if !defined(TMX_MALLOC)
#define TMX_MALLOC(size, user) TMX_REALLOC(NULL, size, user)
#endif

#if !defined(TMX_CALLOC)
static void *
TMC_CALLOC(size_t elemCount, size_t elemSize, TMXuserptr user)
{
    size_t size  = elemCount * elemSize;
    void *memory = TMX_REALLOC(NULL, size, user);
    if (!memory)
        return NULL;
    memset(memory, 0, size);
    return memory;
}
#endif
#endif

static TMXuserptr memoryUserPtr;

void
tmxMemoryUserPtr(TMXuserptr user)
{
    memoryUserPtr = user;
}

void *
tmxMalloc(size_t size)
{
    if (!size)
        return NULL;

    void *ptr = TMX_MALLOC(size, memoryUserPtr);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);

#ifdef TMX_DEBUG
    if (ptr)
        allocationCount++;
#endif
    return ptr;
}

void *
tmxRealloc(void *previous, size_t newSize)
{
    if (!previous && !newSize)
        return NULL;

    void *ptr = TMX_REALLOC(previous, newSize, memoryUserPtr);
    if (!ptr && newSize)
        tmxError(TMX_ERR_MEMORY);

#ifdef TMX_DEBUG
    if (previous && newSize == 0)
        deallocationCount++;
    if (!previous && newSize > 0 && ptr)
        allocationCount++;
#endif

    return ptr;
}

void *
tmxCalloc(size_t elemCount, size_t elemSize)
{
    if (!elemCount || !elemSize)
        return NULL;

    void *ptr = TMX_CALLOC(elemCount, elemSize, memoryUserPtr);
    if (!ptr)
    {
        tmxError(TMX_ERR_MEMORY);
        return NULL;
    }
#ifdef TMX_DEBUG
    allocationCount++;
#endif
    return ptr;
}

void
tmxFree(void *memory)
{
    if (!memory)
        return;
#ifdef TMX_DEBUG
    deallocationCount++;
#endif
    TMX_FREE(memory, memoryUserPtr);
}

static void
tmxFreeProperties(TMXproperties *properties)
{
    if (!properties)
        return;

    struct TMXproperties *entry, *temp;
    HASH_ITER(hh, properties, entry, temp)
    {
        HASH_DEL(properties, entry);
        switch (entry->value.type)
        {
            case TMX_PROPERTY_UNSPECIFIED:
            case TMX_PROPERTY_STRING:
            case TMX_PROPERTY_FILE: tmxFree((void *) entry->value.value.string); break;
            case TMX_PROPERTY_CLASS: tmxFreeProperties(entry->value.value.properties); break;
            default: break; // Nothing to free for other types.
        }
        // The key is the same pointer as the property name, so no need to free it, but it must be freed after
        tmxFree((void *) entry->value.name);
        tmxFree((void *) entry->value.class);
        tmxFree(entry);
    }
}

static void
tmxFreeImage(TMXimage *image)
{
    if (!image)
        return;

    tmxImageUserFree(image);
    tmxFree((void *) image->source);
    tmxFree((void *) image->format);
    tmxFree(image->data);
    tmxFree(image);
}

static void
tmxFreeObject(TMXobject *object)
{
    if (!object)
        return;

    tmxFree((void *) object->name);
    tmxFree((void *) object->class);
    tmxFreeProperties(object->properties);

    if (object->template && !TMX_HAS_FLAG(object->template->flags, TMX_FLAG_CACHED))
        tmxFreeTemplate(object->template);

    switch (object->type)
    {
        case TMX_OBJECT_POLYGON:
        case TMX_OBJECT_POLYLINE: tmxFree(object->poly.points); break;
        case TMX_OBJECT_TEXT:
            if (object->text)
            {
                tmxFree((void *) object->text->font);
                tmxFree((void *) object->text->string);
                tmxFree(object->text);
            }
            break;
        default: break;
    }

    tmxFree(object);
}

static void
tmxFreeLayer(TMXlayer *layer)
{
    if (!layer)
        return;

    size_t i;
    tmxFree((void *) layer->name);
    tmxFree((void *) layer->class);
    tmxFreeProperties(layer->properties);
    switch (layer->type)
    {
        case TMX_LAYER_TILE:
        {
            tmxFree(layer->data.tiles);
            break;
        }
        case TMX_LAYER_CHUNK:
        {
            for (i = 0; i < layer->count; i++)
                tmxFree(layer->data.chunks[i].gids);
            tmxFree(layer->data.chunks);
            break;
        }
        case TMX_LAYER_IMAGE:
        {
            tmxFreeImage(layer->data.image);
            break;
        }
        case TMX_LAYER_OBJGROUP:
        {
            for (i = 0; i < layer->count; i++)
                tmxFreeObject(layer->data.objects[i]);
            tmxFree(layer->data.objects);
            break;
        }
        case TMX_LAYER_GROUP:
        {
            for (i = 0; i < layer->count; i++)
                tmxFreeLayer(layer->data.group[i]);
            tmxFree(layer->data.group);
            break;
        }
    }

    tmxFree(layer);
}

void
tmxFreeMap(TMXmap *map)
{
    if (!map)
        return;

    tmxFree((void *) map->version);
    tmxFree((void *) map->tiled_version);
    tmxFree((void *) map->class);
    tmxFreeProperties(map->properties);

    size_t i;
    for (i = 0; i < map->layer_count; i++)
        tmxFreeLayer(map->layers[i]);

    for (i = 0; i < map->tileset_count; i++)
    {
        if (!map->tilesets[i].tileset || TMX_HAS_FLAG(map->tilesets[i].tileset->flags, TMX_FLAG_CACHED))
            continue;
        tmxFreeTileset(map->tilesets[i].tileset);
    }

    tmxFree(map->layers);
    tmxFree(map->tilesets);
    tmxFree(map);
}

void
tmxFreeTileset(TMXtileset *tileset)
{
    if (!tileset)
        return;

    if (TMX_HAS_FLAG(tileset->flags, TMX_FLAG_CACHED))
    {
        tmxErrorMessage(TMX_ERR_INVALID_OPERATION, "Cannot free cached tileset.");
        return;
    }

    if (tileset->version)
        tmxFree((void *) tileset->version);
    if (tileset->tiled_version)
        tmxFree((void *) tileset->tiled_version);
    if (tileset->name)
        tmxFree((void *) tileset->name);
    if (tileset->class)
        tmxFree((void *) tileset->class);
    if (tileset->image)
        tmxFreeImage(tileset->image);
    if (tileset->properties)
        tmxFreeProperties(tileset->properties);

    size_t i, j;
    TMXtile tile;

    if (tileset->tiles)
    {
        for (i = 0; i < tileset->tile_count; i++)
        {
            tile = tileset->tiles[i];
            if (tile.class)
                tmxFree((void *) tile.class);
            if (tile.image)
                tmxFreeImage(tile.image);
            if (tile.animation.frames)
                tmxFree(tile.animation.frames);
            if (tile.properties)
                tmxFreeProperties(tile.properties);
            if (tile.collision.objects)
            {
                for (j = 0; j < tile.collision.count; j++)
                    tmxFreeObject(tile.collision.objects[j]);
                tmxFree(tile.collision.objects);
            }
        }
        tmxFree(tileset->tiles);
    }

    tmxFree(tileset);
}

void
tmxFreeTemplate(TMXtemplate *template)
{
    if (TMX_HAS_FLAG(template->flags, TMX_FLAG_CACHED))
    {
        tmxErrorMessage(TMX_ERR_INVALID_OPERATION, "Cannot free cached template.");
        return;
    }

    if (template->tileset && !TMX_HAS_FLAG(template->tileset->flags, TMX_FLAG_CACHED))
        tmxFreeTileset(template->tileset);

    tmxFreeObject(template->object);
    tmxFree(template);
}

#ifdef TMX_DEBUG

#include <stdio.h>

#define BLUE   "\033[34m"
#define YELLOW "\033[33m"
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define BRIGHT "\033[1m"
#define RESET  "\033[0m"

void
tmxMemoryLeakCheck(void)
{
    printf("\n" BLUE ":: " RESET BRIGHT "Leak Check\n" RESET);
    printf(YELLOW " -> " RESET "Allocations:    %5zu\n", allocationCount);
    printf(YELLOW " -> " RESET "Deallocations:  %5zu\n", deallocationCount);
    printf(YELLOW " -> " RESET "Result:          " BRIGHT "%s\n\n" RESET, allocationCount == deallocationCount ? GREEN "PASS" : RED "FAIL");
}
#endif
