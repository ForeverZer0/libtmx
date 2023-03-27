#include "cwalk.h"
#include "tmx/memory.h"
#include "internal.h"
#include "tmx/error.h"
#include <ctype.h>
#include <stdlib.h>

#ifdef TMX_DEBUG
static size_t allocationCount;
static size_t deallocationCount;
#endif

#pragma region Default Allocators

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

#pragma endregion /* Default Allocators */

static TMXallocator memoryPool = {tmxMallocImpl, tmxReallocImpl, tmxCallocImpl, tmxFreeImpl};
static TMXuserptr userPtrValue;

#ifndef TMX_LOG_ALLOCATIONS

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
    if (!ptr && newSize)
        tmxError(TMX_ERR_MEMORY);
    return ptr;
}

TMX_INLINE void *
tmxCalloc(size_t elemCount, size_t elemSize)
{
    if (!elemCount || !elemSize)
        return NULL;

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
    if (!memory)
        return;
#ifdef TMX_DEBUG
    deallocationCount++;
#endif
    memoryPool.free(memory, userPtrValue);
}

#else

#include <stdio.h>

void *
tmxMallocLogged(size_t size, const char *file, int line)
{
    if (!size)
        return NULL;
    void *mem = malloc(size);
    printf("ALLOCATE: %p - %s - %d\n", mem, file, line);
    return mem;
}

void *
tmxReallocLogged(void *previous, size_t size, const char *file, int line)
{
    const char *word = NULL;
    if (previous == NULL)
        word = "ALLOCATE";
    else if (size == 0)
        word = "DEALLOCATE";

    void *mem = realloc(previous, size);

    if (word)
        printf("%s: %p - %s - %d", word, mem, file, line);

    return mem;
}

void *
tmxCallocLogged(size_t count, size_t size, const char *file, int line)
{
    void *mem = calloc(count, size);
    printf("ALLOCATE: %p - %s - %d\n", mem, file, line);
    return mem;
}

void
tmxFreeLogged(void *ptr, const char *file, int line)
{
    if (!ptr)
        return;

    printf("DEALLOCATE: %p - %s - %d\n", ptr, file, line);
    free(ptr);
}

#endif

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

    if (object->template && !TMX_FLAG(object->template->flags, TMX_FLAG_CACHED))
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
        if (!map->tilesets[i].tileset || TMX_FLAG(map->tilesets[i].tileset->flags, TMX_FLAG_CACHED))
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

    if (TMX_FLAG(tileset->flags, TMX_FLAG_CACHED))
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


static TMXreadfunc fileRead;
static TMXfreefunc fileFree;
static TMXuserptr fileUserPtr;

size_t tmxFileAbsolutePath(const char *path, const char *basePath, char *buffer, size_t bufferSize)
{   
    size_t dirLen;;
    cwk_path_get_dirname(basePath, &dirLen);

    char dirBuffer[dirLen + 1];
    memcpy(dirBuffer, basePath, dirLen);
    dirBuffer[dirLen] = '\0';

    return cwk_path_get_absolute(dirBuffer, path, buffer, bufferSize);
}


size_t tmxFileDirectory(const char *path)
{
    const char *fs = strrchr(path, '/');
    const char *bs = strrchr(path, '\\');

    const char *last = TMX_MAX(fs, bs);
    if (!last)
        return 0;

    return (size_t)((last + 1) - path);
}

static char *tmxFileReadImpl(const char *path, const char *basePath)
{
    char *result = NULL;
    size_t len;

    FILE *fp = NULL;
    fp = fopen(path, "r");

    if (!fp && basePath)
    {
        char buffer[TMX_MAX_PATH];
        len = tmxFileAbsolutePath(path, basePath, buffer, TMX_MAX_PATH);
        if (!len)
            return NULL;

        fp = fopen(buffer, "r");
    }

    if (!fp)
        return NULL;

    fseek(fp, 0, SEEK_END);
    len = (size_t) ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (len)
    {
        result = tmxMalloc(len + 1);
        if (fread(result, 1, len, fp) != len)
        {
            tmxErrorFormat(TMX_ERR_IO, "Failed to read from \"%s\".", path);
            tmxFree(result);
            result = NULL;
        }
        else
        {
            result[len] = '\0';
        }
    }

    fclose(fp);
    return result;
}

char *tmxFileRead(const char *path, const char *basePath)
{
    if (!path)
        return NULL;

    if (fileRead)
    {
        char *result = NULL;
        size_t len;

        const char *userBuffer = fileRead(path, basePath, fileUserPtr);
        if (userBuffer)
        {
            len = strlen(userBuffer);
            result = tmxMalloc(len + 1);
            memcpy(result, userBuffer, len);
            result[len] = '\0';
            if (fileFree)
                fileFree((void*)userBuffer, fileUserPtr);
            return result;
        }
    }

    return tmxFileReadImpl(path, basePath);
}