
#include "tmx/memory.h"
#include "tmx/error.h"
#include "internal.h"
#include <stdlib.h>

#ifdef TMX_DEBUG
static size_t allocationCount;
static size_t deallocationCount;
#endif

static void *
tmxMallocImpl(size_t size, TMXuserptr user)
{
    TMX_UNUSED(user);
    return malloc(size);
}

static void *
tmxReallocImpl(void *previous, size_t newSize, TMXuserptr user)
{
    TMX_UNUSED(user);
    return realloc(previous, newSize);
}

static void *
tmxCallocImpl(size_t elemCount, size_t elemSize, TMXuserptr user)
{
    TMX_UNUSED(user);
    return calloc(elemCount, elemSize);
}

static void
tmxFreeImpl(void *memory, TMXuserptr user)
{
    TMX_UNUSED(user);
    free(memory);
}

static TMXallocator memoryPool = {tmxMallocImpl, tmxReallocImpl, tmxCallocImpl, tmxFreeImpl};
static TMXuserptr userPtrValue;

TMX_INLINE void *
tmxMalloc(size_t size)
{
    if (!size)
        return NULL;
#ifdef TMX_DEBUG
    allocationCount++;
#endif
    void *ptr = memoryPool.malloc(size, userPtrValue);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

TMX_INLINE void *
tmxRealloc(void *previous, size_t newSize)
{
#ifdef TMX_DEBUG
    if (!previous)
        allocationCount++;
    else if (!newSize)
        deallocationCount++;
#endif

    void *ptr = memoryPool.realloc(previous, newSize, userPtrValue);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

TMX_INLINE void *
tmxCalloc(size_t elemCount, size_t elemSize)
{
#ifdef TMX_DEBUG
    allocationCount++;
#endif
    void *ptr = memoryPool.calloc(elemCount, elemSize, userPtrValue);
    if (!ptr)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

TMX_INLINE void
tmxFree(void *memory)
{
#ifdef TMX_DEBUG
    if (memory)
        deallocationCount++;
#endif
    memoryPool.free(memory, userPtrValue);
}

void tmxFreeMap(TMXmap *map)
{
    if (!map)
        return;

    TMXlayer *layer, *tempLayer;
    TMXmaptileset *tileset, *tempTileset;

    tmxFree((void*)map->version);
    tmxFree((void*)map->tiled_version);
    tmxFree((void*)map->class);
    tmxFreeProperties(map->properties);

    layer = map->layers;
    while (layer)
    {
        tempLayer = layer->next;
        tmxFreeLayer(layer);
        layer = tempLayer;
    }

    tileset = map->tilesets;
    while (tileset)
    {
        tempTileset = tileset->next;
        if (!TMX_FLAG(tileset->tileset->flags, TMX_FLAG_CACHED))
        {
            tmxFreeTileset(tileset->tileset);
            tmxFree(tileset);
        }
        tileset = tempTileset;
    }

    tmxFree(map);
}

void tmxFreeTileset(TMXtileset *tileset)
{   
    if (!tileset)
        return;

    if (TMX_FLAG(tileset->flags, TMX_FLAG_CACHED))
    {
        tmxErrorMessage(TMX_ERR_INVALID_OPERATION, "Cannot free cached tileset.");
        return;
    }

    if (tileset->version)
        tmxFree((void*) tileset->version);
    if (tileset->tiled_version)
        tmxFree((void*)tileset->tiled_version);
    if (tileset->name)
        tmxFree((void*) tileset->name);
    if (tileset->class)
        tmxFree((void*) tileset->class);
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
                tmxFree((void*) tile.class);
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

void tmxFreeTemplate(TMXtemplate *template)
{
    if (TMX_FLAG(template->flags, TMX_FLAG_CACHED))
    {
        tmxErrorMessage(TMX_ERR_INVALID_OPERATION, "Cannot free cached template.");
        return;
    }

    if (template->tileset && !TMX_FLAG(template->tileset->flags, TMX_FLAG_CACHED))
        tmxFreeTileset(template->tileset);

    tmxFreeObject(template->object);
    tmxFree(template);
}

void
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
            case TMX_UNSPECIFIED:
            case TMX_PROPERTY_STRING:
            case TMX_PROPERTY_FILE: tmxFree((void *) entry->value.value.string); break;
            case TMX_PROPERTY_CLASS: tmxFreeProperties(entry->value.value.properties); break;
        }
        // The key is the same pointer as the property name, so no need to free it, but it must be freed after
        tmxFree((void *) entry->value.name);
        tmxFree((void *) entry->value.custom_type);
        tmxFree(entry);
    }
}

void tmxFreeLayer(TMXlayer *layer)
{
    if (!layer)
        return;

    size_t i;
    tmxFree((void*)layer->name);
    tmxFree((void*)layer->class);
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
            TMXobject *temp, *obj = layer->data.objects;
            while (obj)
            {
                temp = obj->next;
                tmxFreeObject(obj);
                obj = temp;
            }
            break;
        }
        case TMX_LAYER_GROUP:
        {
            TMXlayer *temp, *child = layer->data.group;
            while (child)
            {
                temp = child->next;
                tmxFreeLayer(child);
                child = temp;
            }
            break;
        }
    }

    tmxFree(layer);
}

void tmxFreeObject(TMXobject *object)
{
    if (!object)
        return;

    tmxFree((void*)object->name);
    tmxFree((void*)object->class);
    tmxFreeProperties(object->properties);
    
    if (object->template && !TMX_FLAG(object->template->flags, TMX_FLAG_CACHED))
        tmxFreeTemplate(object->template);

    switch (object->type)
    {
        case TMX_OBJECT_POLYGON:
        case TMX_OBJECT_POLYLINE:
            tmxFree(object->poly.points);
            break;
        case TMX_OBJECT_TEXT:
            tmxFree((void*) object->text->font);
            tmxFree((void*) object->text->string);
            tmxFree(object->text);
            break;
        default: break;
    }

    tmxFree(object);
}

void tmxFreeImage(TMXimage *image)
{
    if (!image)
        return;

    if (TMX_FLAG(image->flags, TMX_FLAG_CACHED))
    {
        tmxErrorMessage(TMX_ERR_INVALID_OPERATION, "Cannot free cached image.");
        return;
    }

    tmxImageUserFree(image);
    tmxFree((void*)image->source);
    tmxFree((void*)image->format);
    tmxFree(image->data);
    tmxFree(image);
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