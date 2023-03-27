#include "cwalk.h"
#include "internal.h"
#include "parse.h"
#include "tmx/cache.h"
#include "tmx/compression.h"
#include "tmx/properties.h"
#include "tmx/xml.h"
#include <stdio.h>

typedef struct
{
    TMXcache *cache;
    TMXmap *map;
    const char *base_path;
    TMXxmlreader *reader;
    char *text;
    TMXbool freeText;
} TMXxml;

#if defined(TMX_WARN_UNHANDLED)
static void
tmxXmlWarnElement(const char *parent, const char *elemName)
{
    tmxErrorFormat(TMX_ERR_WARN, "Unhandled child element <%s> in <%s>.", elemName, parent);
}

static void
tmxXmlWarnAttribute(const char *parent, const char *attrName)
{
    tmxErrorFormat(TMX_ERR_WARN, "Unhandled child attribute \"%s\" in <%s>.", attrName, parent);
}
#else
#define tmxXmlWarnElement(parent, elemName)
#define tmxXmlWarnAttribute(parent, attrName)
#endif

static TMX_INLINE void
tmxParsePoints(char *value, struct TMXcoords *coords)
{
    static const char *delim = " ";

    // Not CSV, but the format has as many commas as there are values, so it works well.
    coords->count  = tmxCsvCount(value, strlen(value));
    size_t i       = 0;
    char *token    = strtok(value, delim);
    coords->points = tmxCalloc(coords->count, sizeof(TMXvec2));

    while (token && i < coords->count)
    {
        if (sscanf(token, "%f,%f", &coords->points[i].x, &coords->points[i].y) != 2)
        {
            tmxFree(coords->points);
            coords->count = 0;
            tmxErrorMessage(TMX_ERR_PARSE, "Malformed points list.");
            return;
        }
        i++;
        token = strtok(NULL, delim);
    }
}

static void
tmxXmlParseDataType(TMXcontext *context, TMXenum *encoding, TMXenum *compression)
{
    *encoding    = TMX_ENCODING_NONE;
    *compression = TMX_COMPRESSION_NONE;

    const char *name;
    const char *value;

    if (!tmxXmlAssertElement(context->xml, WORD_DATA))
        return;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_ENCODING))
            *encoding = tmxParseEncoding(value);
        else if (STREQL(name, WORD_COMPRESSION))
            *compression = tmxParseCompression(value);
        else
        {
            tmxXmlWarnAttribute(WORD_DATA, name);
        }
    }
}

static TMXproperties *
tmxXmlParseProperties(TMXcontext *context)
{
    const char *name;
    const char *value;
    size_t size;
    TMXproperties *entry, *properties = NULL;
    TMXproperty *property;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, WORD_PROPERTY))
            continue;

        entry    = tmxCalloc(1, sizeof(TMXproperties));
        property = &entry->value;
        while (tmxXmlReadAttr(context->xml, &name, &value))
        {
            if (STREQL(name, WORD_NAME))
                property->name = tmxStringDup(value);
            else if (STREQL(name, WORD_TYPE))
                property->type = tmxParsePropertyType(value);
            else if (STREQL(name, WORD_PROPERTY_TYPE))
                property->custom_type = value;
            else if (STREQL(name, WORD_VALUE))
            {
                // The type is always defined before the value.
                switch (property->type)
                {
                    case TMX_UNSPECIFIED:
                    case TMX_PROPERTY_FILE:
                    case TMX_PROPERTY_STRING: property->value.string = tmxStringDup(value); break;
                    case TMX_PROPERTY_INTEGER:
                    case TMX_PROPERTY_OBJECT: property->value.integer = TMX_STR2INT(value); break;
                    case TMX_PROPERTY_FLOAT: property->value.decimal = TMX_STR2FLT(value); break;
                    case TMX_PROPERTY_BOOL: property->value.integer = TMX_STR2BOOL(value); break;
                    case TMX_PROPERTY_COLOR: property->value.color = tmxParseColor(value); break;
                    case TMX_PROPERTY_CLASS: break;
                }
            }
            else
            {
                tmxXmlWarnAttribute(WORD_PROPERTY, name);
            }
        }

        if (tmxXmlMoveToContent(context->xml))
            property->value.properties = tmxXmlParseProperties(context);

        entry->key = property->name;
        HASH_ADD_KEYPTR(hh, properties, entry->key, strlen(entry->key), entry);

        TMXproperties *previous = entry->hh.prev;
        if (previous)
            previous->value.next = property;
    }
    return properties;
}

static TMXimage *
tmxXmlParseImage(TMXcontext *context)
{
    const char *name;
    const char *value;
    size_t size;
    TMXimage *image = tmxCalloc(1, sizeof(TMXimage));

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_FORMAT))
            image->format = tmxStringDup(value);
        else if (STREQL(name, WORD_SOURCE))
        {
            image->source = tmxStringDup(value);
            image->flags |= TMX_FLAG_EXTERNAL;
        }
        else if (STREQL(name, WORD_TRANS))
        {
            image->transparent = tmxParseColor(value);
            image->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, WORD_WIDTH))
            image->size.w = TMX_STR2INT(value);
        else if (STREQL(name, WORD_HEIGHT))
            image->size.h = TMX_STR2INT(value);
        else
        {
            tmxXmlWarnAttribute(WORD_IMAGE, name);
        }
    }

    if (!tmxXmlMoveToContent(context->xml))
        return image;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, WORD_DATA))
            continue;

        image->flags |= TMX_FLAG_EMBEDDED;

        TMXenum encoding, compression;
        tmxXmlParseDataType(context, &encoding, &compression);
        tmxXmlMoveToContent(context->xml);

        if (compression != TMX_COMPRESSION_NONE)
        {
            tmxErrorMessage(TMX_ERR_UNSUPPORTED, "Compressed image data is not supported.");
            break;
        }

        const char *contents;
        if (tmxXmlReadStringContents(context->xml, &contents, &size, TMX_TRUE))
        {
            size_t dataSize = tmxBase64DecodedSize(contents, size);
            image->data     = tmxMalloc(dataSize);
            if (!image->data)
                break;
            tmxBase64Decode(contents, size, image->data, dataSize);
        }
        break;
    }

    tmxImageUserLoad(image, context->basePath);
    return image;
}

static struct TMXtext *
tmxXmlParseObjectText(TMXcontext *context, TMXobject *obj)
{
    struct TMXtext *text = TMX_ALLOC(struct TMXtext);

    const char *name;
    const char *value;
    size_t size;

    // Set defaults
    TMXflag halign   = TMX_ALIGN_LEFT;
    TMXflag valign   = TMX_ALIGN_TOP;
    text->pixel_size = 16;
    text->kerning    = TMX_TRUE;
    text->wrap       = TMX_TRUE;
#ifdef TMX_VECTOR_COLOR
    text->color.a = 1.0f;
#else
    text->color.a = 255;
#endif

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_FONT_FAMILY))
        {
            text->font = tmxStringDup(value);
            obj->flags |= TMX_FLAG_FONT;
        }
        else if (STREQL(name, WORD_PIXEL_SIZE))
        {
            text->pixel_size = TMX_STR2INT(value);
            obj->flags |= TMX_FLAG_FONT_SIZE;
        }
        else if (STREQL(name, WORD_WRAP))
        {
            text->wrap = TMX_STR2BOOL(value);
            obj->flags |= TMX_FLAG_WORD_WRAP;
        }
        else if (STREQL(name, WORD_COLOR))
        {
            text->color = tmxParseColor(value);
            obj->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, WORD_BOLD))
        {
            text->style |= TMX_FONT_STYLE_BOLD;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_BOLD);
        }
        else if (STREQL(name, WORD_ITALIC))
        {
            text->style |= TMX_FONT_STYLE_ITALIC;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_ITALIC);
        }
        else if (STREQL(name, WORD_UNDERLINE))
        {
            text->style |= TMX_FONT_STYLE_UNDERLINE;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_UNDERLINE);
        }
        else if (STREQL(name, WORD_STRIKEOUT))
        {
            text->style |= TMX_FONT_STYLE_STRIKEOUT;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_STRIKEOUT);
        }
        else if (STREQL(name, WORD_KERNING))
        {
            text->kerning = TMX_STR2BOOL(value);
            obj->flags |= TMX_FLAG_FONT_KERNING;
        }
        else if (STREQL(name, WORD_HALIGN))
        {
            halign = tmxParseAlignH(value);
            obj->flags |= (TMX_FLAG_ALIGN | TMX_FLAG_HALIGN);
        }
        else if (STREQL(name, WORD_VALIGN))
        {
            valign = tmxParseAlignV(value);
            obj->flags |= (TMX_FLAG_ALIGN | TMX_FLAG_VALIGN);
        }
        else
        {
            tmxXmlWarnAttribute(WORD_TEXT, name);
        }
    }
    text->align = (halign | valign);

    if (tmxXmlMoveToContent(context->xml))
    {
        if (tmxXmlReadStringContents(context->xml, &value, &size, TMX_FALSE))
        {
            text->string = tmxStringCopy(value, size);
            obj->flags |= TMX_FLAG_TEXT;
        }
    }

    return text;
}

static TMXobject *
tmxXmlParseObject(TMXcontext *context)
{
    TMXobject *object = TMX_ALLOC(TMXobject);

    const char *name;
    const char *value;
    size_t size;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_ID))
            object->id = TMX_STR2INT(value);
        else if (STREQL(name, WORD_NAME))
        {
            object->name = tmxStringDup(value);
            object->flags |= TMX_FLAG_NAME;
        }
        else if (STREQL(name, WORD_TYPE))
            object->class = tmxStringDup(value);
        else if (STREQL(name, WORD_X))
        {
            object->position.x = TMX_STR2FLT(value);
            object->flags |= (TMX_FLAG_POSITION | TMX_FLAG_X);
        }
        else if (STREQL(name, WORD_Y))
        {
            object->position.y = TMX_STR2FLT(value);
            object->flags |= (TMX_FLAG_POSITION | TMX_FLAG_Y);
        }
        else if (STREQL(name, WORD_WIDTH))
        {
            object->size.x = TMX_STR2FLT(value);
            object->flags |= (TMX_FLAG_SIZE | TMX_FLAG_WIDTH);
        }
        else if (STREQL(name, WORD_HEIGHT))
        {
            object->size.y = TMX_STR2FLT(value);
            object->flags |= (TMX_FLAG_SIZE | TMX_FLAG_HEIGHT);
        }
        else if (STREQL(name, WORD_ROTATION))
        {
            object->rotation = TMX_STR2FLT(value);
            object->flags |= TMX_FLAG_ROTATION;
        }
        else if (STREQL(name, WORD_GID))
        {
            char *end   = NULL;
            object->gid = (TMXgid) strtoul(value, &end, 10);
            object->flags |= TMX_FLAG_GID;
        }
        else if (STREQL(name, WORD_VISIBLE))
        {
            object->visible = TMX_STR2BOOL(value);
            object->flags |= TMX_FLAG_VISIBLE;
        }
        else if (STREQL(name, WORD_TYPE) || STREQL(name, WORD_CLASS))
        {
            object->flags |= TMX_FLAG_CLASS;
            object->class = tmxStringDup(value);
        }
        else if (STREQL(name, WORD_TEMPLATE))
        {
            char templatePath[TMX_MAX_PATH];
            tmxFileAbsolutePath(value, context->basePath, templatePath, TMX_MAX_PATH);
            object->template = tmxLoadTemplate(templatePath, context->cache, TMX_FORMAT_AUTO);
        }
        else
        {
            tmxXmlWarnAttribute(WORD_OBJECT, name);
        }
    }

    // Move to contents. Return early if there is none.
    if (!tmxXmlMoveToContent(context->xml))
        return object;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, WORD_PROPERTIES))
        {
            object->properties = tmxXmlParseProperties(context);
            object->flags |= TMX_FLAG_PROPERTIES;
            continue;
        }
        else if (STREQL(name, WORD_POINT))
        {
            object->type = TMX_OBJECT_POINT;
        }
        else if (STREQL(name, WORD_ELLIPSE))
        {
            object->type = TMX_OBJECT_ELLIPSE;
        }
        else if (STREQL(name, WORD_POLYGON) || STREQL(name, WORD_POLYLINE))
        {
            object->type = STREQL(name, WORD_POLYGON) ? TMX_OBJECT_POLYGON : TMX_OBJECT_POLYLINE;
            tmxXmlReadAttr(context->xml, &name, &value);
            tmxXmlMoveToContent(context->xml);
            // It is safe to stomp all over this pointer, it is temporary and no longer valid on the next loop
            tmxParsePoints((char *) value, &object->poly);
            object->flags |= TMX_FLAG_POINTS;
        }
        else if (STREQL(name, WORD_TEXT))
        {
            object->type = TMX_OBJECT_TEXT;
            object->text = tmxXmlParseObjectText(context, object);
        }
        else
        {
            tmxXmlWarnElement(WORD_OBJECT, name);
            tmxXmlSkipElement(context->xml);
        }
    }

    if (object->template && object->template->object)
        tmxObjectMergeTemplate(object, object->template->object);

    return object;
}

static void
tmxXmlParseTileIds(TMXcontext *context, TMXenum encoding, TMXenum compression, TMXgid *output, size_t outputCount)
{
    const char *str;
    size_t strSize;
    tmxXmlMoveToContent(context->xml);

    if (encoding == TMX_ENCODING_NONE)
    {
        size_t i = 0;
        TMXgid gid;
        const char *value;
        char *end = NULL;

        while (tmxXmlReadElement(context->xml, &str, &strSize))
        {

            if (!STREQL(str, WORD_TILE))
            {
                tmxXmlWarnElement("data/chunk", str);
                tmxXmlSkipElement(context->xml);
                continue;
            }

            gid = 0;
            while (tmxXmlReadAttr(context->xml, &str, &value))
            {
                if (STREQL(str, WORD_ID))
                {
                    gid = strtoul(str, &end, 10);
                }
                else
                {
                    tmxXmlWarnAttribute(WORD_TILE, str);
                }
            }
            output[i++] = gid;
        }
        return;
    }
    tmxXmlReadStringContents(context->xml, &str, &strSize, TMX_TRUE);

    if (encoding == TMX_ENCODING_CSV)
    {
        tmxCsvDecode(str, strSize, output, outputCount);
        return;
    }

    tmxInflate(str, strSize, output, outputCount, compression);
}

static void
tmxXmlParseTileData(TMXcontext *context, TMXlayer *layer)
{
    const char *name;
    const char *value;
    size_t nameSize;
    TMXenum encoding, compression;
    tmxXmlParseDataType(context, &encoding, &compression);

    if (context->map->infinite)
    {
        TMXchunk *chunk;
        size_t capacity    = 16;
        layer->data.chunks = tmxMalloc(capacity * sizeof(TMXchunk));
        tmxXmlMoveToContent(context->xml);

        while (tmxXmlReadElement(context->xml, &name, &nameSize))
        {
            if (layer->count >= capacity)
            {
                capacity *= 2;
                layer->data.chunks = tmxRealloc(layer->data.chunks, capacity * sizeof(TMXchunk));
            }

            chunk = &layer->data.chunks[layer->count];
            memset(chunk, 0, sizeof(TMXchunk));

            while (tmxXmlReadAttr(context->xml, &name, &value))
            {
                if (STREQL(name, WORD_X))
                    chunk->bounds.x = TMX_STR2INT(value);
                else if (STREQL(name, WORD_Y))
                    chunk->bounds.y = TMX_STR2INT(value);
                else if (STREQL(name, WORD_WIDTH))
                    chunk->bounds.w = TMX_STR2INT(value);
                else if (STREQL(name, WORD_HEIGHT))
                    chunk->bounds.h = TMX_STR2INT(value);
                else
                {
                    tmxXmlWarnAttribute(WORD_CHUNK, name);
                }
            }

            chunk->count = chunk->bounds.w * chunk->bounds.h;
            chunk->gids  = tmxCalloc(chunk->count, sizeof(TMXgid));
            tmxXmlParseTileIds(context, encoding, compression, chunk->gids, chunk->count);
            layer->count++;
            tmxXmlMoveToContent(context->xml);
        }

        if (layer->count < capacity)
            layer->data.chunks = tmxRealloc(layer->data.chunks, layer->count * sizeof(TMXchunk));
    }
    else
    {
        layer->count      = context->map->size.w * context->map->size.h;
        layer->data.tiles = tmxCalloc(layer->count, sizeof(TMXgid));
        tmxXmlParseTileIds(context, encoding, compression, layer->data.tiles, layer->count);
    }
}

static TMXlayer *
tmxXmlParseLayer(TMXcontext *context, const char *layerType)
{
    const char *name;
    const char *value;
    size_t size;

    TMXlayer *layer = TMX_ALLOC(TMXlayer);
    layer->type     = tmxParseLayerType(layerType, context->map->infinite);
    layer->parallax = (TMXvec2){1.0f, 1.0f};
    layer->visible  = TMX_TRUE;
    layer->opacity  = 1.0f;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_ID))
            layer->id = TMX_STR2INT(value);
        else if (STREQL(name, WORD_NAME))
            layer->name = tmxStringDup(value);
        else if (STREQL(name, WORD_CLASS))
            layer->class = tmxStringDup(value);
        else if (STREQL(name, WORD_X))
            layer->position.x = TMX_STR2INT(value);
        else if (STREQL(name, WORD_Y))
            layer->position.y = TMX_STR2INT(value);
        else if (STREQL(name, WORD_WIDTH))
            layer->size.w = TMX_STR2INT(value);
        else if (STREQL(name, WORD_HEIGHT))
            layer->size.h = TMX_STR2INT(value);
        else if (STREQL(name, WORD_OPACITY))
            layer->opacity = TMX_STR2FLT(value);
        else if (STREQL(name, WORD_VISIBLE))
            layer->visible = TMX_STR2BOOL(value);
        else if (STREQL(name, WORD_OFFSET_X))
            layer->offset.x = TMX_STR2INT(value);
        else if (STREQL(name, WORD_OFFSET_Y))
            layer->offset.y = TMX_STR2INT(value);
        else if (STREQL(name, WORD_PARALLAX_X))
            layer->parallax.x = TMX_STR2FLT(value);
        else if (STREQL(name, WORD_PARALLAX_Y))
            layer->parallax.y = TMX_STR2FLT(value);
        else if (STREQL(name, WORD_TINT_COLOR))
        {
            layer->tint_color = tmxParseColor(value);
            layer->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, WORD_DRAW_ORDER)) // <objectgroup> only
            layer->draw_order = tmxParseDrawOrder(value);
        else if (STREQL(name, WORD_REPEAT_X)) // <imagelayer> only
            layer->repeat.x = TMX_STR2BOOL(value);
        else if (STREQL(name, WORD_REPEAT_Y)) // <imagelayer> only
            layer->repeat.y = TMX_STR2BOOL(value);
        else
        {
            tmxXmlWarnAttribute(layerType, name);
        }
    }

    if (!tmxXmlMoveToContent(context->xml))
    {
        tmxError(TMX_ERR_PARSE);
        return layer;
    }

    size_t objectCapa = 8;
    size_t layerCapa  = 4;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, WORD_PROPERTIES))
        {
            layer->properties = tmxXmlParseProperties(context);
            layer->flags |= TMX_FLAG_PROPERTIES;
        }
        else if (STREQL(name, WORD_DATA)) // <layer>
        {
            tmxXmlMoveToContent(context->xml);
            tmxXmlParseTileData(context, layer);
        }
        else if (STREQL(name, WORD_OBJECT)) // <objectgroup>
        {
            TMXobject *obj = tmxXmlParseObject(context);
            if (!obj)
            {
                tmxErrorMessage(TMX_ERR_PARSE, "Failed to parse map object.");
                continue;
            }

            if (!layer->data.objects)
                layer->data.objects = tmxMalloc(objectCapa * sizeof(TMXobject *));
            tmxArrayPush(TMXobject *, layer->data.objects, obj, layer->count, layerCapa);
        }
        else if (STREQL(name, WORD_IMAGE)) // <imagelayer>
        {
            layer->data.image = tmxXmlParseImage(context);
        }
        else if (STREQL(name, WORD_LAYER) || STREQL(name, WORD_OBJECT_GROUP) || STREQL(name, WORD_IMAGE_LAYER) ||
                 STREQL(name, WORD_GROUP)) // <group>
        {
            TMXlayer *child = tmxXmlParseLayer(context, name);
            if (!child)
                continue;

            if (!layer->data.group)
                layer->data.group = tmxMalloc(layerCapa * sizeof(TMXlayer *));
            tmxArrayPush(TMXlayer *, layer->data.group, child, layer->count, layerCapa);
        }
        else
        {
            tmxXmlWarnElement(layerType, name);
            tmxXmlSkipElement(context->xml);
        }
    }

    if (layer->type == TMX_LAYER_OBJGROUP && layer->data.objects)
        tmxArrayFinish(TMXobject *, layer->data.objects, layer->count, objectCapa);
    else if (layer->type == TMX_LAYER_GROUP && layer->data.group)
        tmxArrayFinish(TMXlayer *, layer->data.group, layer->count, layerCapa);

    return layer;
}

static void
tmxParseAnimation(TMXcontext *context, TMXanimation *animation)
{
    TMXframe *frame;
    const char *name;
    const char *value;
    char *end = NULL;
    size_t size;
    size_t capacity   = 8;
    animation->frames = tmxMalloc(capacity * sizeof(TMXframe));

    tmxXmlMoveToContent(context->xml);
    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, WORD_FRAME))
        {
            tmxXmlWarnElement(WORD_MAP, name);
            tmxXmlSkipElement(context->xml);
            continue;
        }

        if (animation->count >= capacity)
        {
            capacity *= 2;
            animation->frames = tmxRealloc(animation->frames, capacity * sizeof(TMXframe));
        }

        frame = &animation->frames[animation->count++];
        while (tmxXmlReadAttr(context->xml, &name, &value))
        {
            if (STREQL(name, WORD_TILE_ID))
                frame->id = (TMXtid) strtoul(value, &end, 10);
            else if (STREQL(name, WORD_DURATION))
                frame->duration = strtoul(value, &end, 10);
        }
        tmxXmlMoveToContent(context->xml);
    }

    if (capacity > animation->count)
        animation->frames = tmxRealloc(animation->frames, animation->count * sizeof(TMXframe));
}

static void
tmxParseCollision(TMXcontext *context, TMXcollision *collision)
{
    const char *name;
    size_t size;
    size_t capacity = 4;
    TMXobject *object;
    collision->objects = tmxMalloc(capacity * sizeof(TMXobject *));

    tmxXmlMoveToContent(context->xml);
    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, WORD_OBJECT))
        {
            tmxXmlWarnElement(WORD_OBJECT_GROUP, name);
            tmxXmlSkipElement(context->xml);
            continue;
        }

        object = tmxXmlParseObject(context);
        if (!object)
            continue;
        tmxArrayPush(TMXobject *, collision->objects, object, collision->count, capacity);
    }
    tmxArrayFinish(TMXobject *, collision->objects, collision->count, capacity);
}

static void
tmxInitTilesetTiles(TMXtileset *tileset, TMXbool isCollection)
{
    if (!tileset->tile_count)
        return;

    int x, y;
    tileset->tiles = tmxCalloc(tileset->tile_count, sizeof(TMXtile));

    // A "classic" tileset based on a single image with uniform tiles
    if (!isCollection)
    {
        size_t i;
        for (i = 0; i < tileset->tile_count; i++)
        {
            x = (i % tileset->columns) * tileset->tile_size.w;
            y = (i / tileset->columns) * tileset->tile_size.h;

            tileset->tiles[i].id   = (TMXtid) i;
            tileset->tiles[i].rect = (TMXrect){.x=x, .y=y, .w=tileset->tile_size.w, .h=tileset->tile_size.h};
        }
    }
}

static void
tmxXmlParseTile(TMXcontext *context, TMXtile *tiles, TMXbool isCollection, size_t tileIndex)
{
    const char *name;
    const char *value;
    size_t size;
    char *end = NULL;

    TMXtile *tile = NULL;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_ID))
        {
            TMXtid id = (TMXtid) strtoul(value, &end, 10);
            if (!isCollection)
                tileIndex = id;
            tile     = &tiles[tileIndex];
            tile->id = id;
        }
        else if (STREQL(name, WORD_TYPE))
            tile->class = tmxStringDup(value);
        else if (STREQL(name, WORD_X))
            tile->rect.x = TMX_STR2INT(value);
        else if (STREQL(name, WORD_Y))
            tile->rect.y = TMX_STR2INT(value);
        else if (STREQL(name, WORD_WIDTH))
            tile->rect.w = TMX_STR2INT(value);
        else if (STREQL(name, WORD_HEIGHT))
            tile->rect.h = TMX_STR2INT(value);
        else
        {
            tmxXmlWarnAttribute(WORD_TILE, name);
        }
    }

    if (!tmxXmlMoveToContent(context->xml))
        return;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, WORD_ANIMATION))
            tmxParseAnimation(context, &tile->animation);
        else if (STREQL(name, WORD_OBJECT_GROUP))
            tmxParseCollision(context, &tile->collision);
        else if (STREQL(name, WORD_IMAGE))
            tile->image = tmxXmlParseImage(context);
        else if (STREQL(name, WORD_PROPERTIES))
            tile->properties = tmxXmlParseProperties(context);
        else
        {
            tmxXmlWarnElement(WORD_TILE, name);
            tmxXmlSkipElement(context->xml);
        }
    }
}

static TMXtileset *
tmxXmlParseTileset(TMXcontext *context, TMXgid *firstGid)
{
    TMXtileset *tileset = NULL;
    const char *name;
    const char *value;
    char *end = NULL;
    size_t size;

    // The XML spec for external/embedded tilesets combined with a forward-only parser makes this a bit wonky.

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_FIRST_GID))
        {
            if (firstGid)
                *firstGid = strtoul(value, &end, 10);
            continue;
        }
        else if (STREQL(name, WORD_SOURCE))
        {
            char buffer[TMX_MAX_PATH];
            tmxFileAbsolutePath(value, context->basePath, buffer, TMX_MAX_PATH);
            tmxXmlMoveToContent(context->xml);
            if (tmxCacheTryGetTileset(context->cache, buffer, &tileset))
                return tileset;

            tileset = tmxLoadTileset(buffer, context->cache, TMX_FORMAT_AUTO);
            return tileset;
        }

        if (!tileset)
            tileset = TMX_ALLOC(TMXtileset);

        // The "firstgid" and "source" would be first/only attributes in an external tileset, so if neither are defined by
        // this point, then this is the actual tileset definition, and it needs allocated before reading anything more.

        if (STREQL(name, WORD_NAME))
            tileset->name = tmxStringDup(value);
        else if (STREQL(name, WORD_CLASS))
            tileset->class = tmxStringDup(value);
        else if (STREQL(name, WORD_TILE_WIDTH))
            tileset->tile_size.w = TMX_STR2INT(value);
        else if (STREQL(name, WORD_TILE_HEIGHT))
            tileset->tile_size.h = TMX_STR2INT(value);
        else if (STREQL(name, WORD_SPACING))
            tileset->spacing = TMX_STR2INT(value);
        else if (STREQL(name, WORD_MARGIN))
            tileset->margin = TMX_STR2INT(value);
        else if (STREQL(name, WORD_TILE_COUNT))
            tileset->tile_count = strtoul(value, &end, 10);
        else if (STREQL(name, WORD_COLUMNS))
            tileset->columns = TMX_STR2INT(value);
        else if (STREQL(name, WORD_OBJECT_ALIGN))
            tileset->object_align = tmxParseObjectAlignment(value);
        else if (STREQL(name, WORD_TILE_RENDER_SIZE))
            tileset->render_size = tmxParseRenderSize(value);
        else if (STREQL(name, WORD_FILL_MODE))
            tileset->fill_mode = tmxParseFillMode(value);
        else if (STREQL(name, WORD_VERSION))
            tileset->version = tmxStringDup(value);
        else if (STREQL(name, WORD_TILED_VERSION))
            tileset->tiled_version = tmxStringDup(value);
        else
        {
            tmxXmlWarnAttribute(WORD_TILESET, name);
        }
    }

    if (!tileset->version && context->map)
        tileset->version = tmxStringDup(context->map->version);
    if (!tileset->tiled_version && context->map)
        tileset->tiled_version = tmxStringDup(context->map->tiled_version);

    // Set defaults for object alignment depending on map orientation if was not specified
    if (tileset->object_align == TMX_UNSPECIFIED && context->map)
    {
        if (context->map->orientation == TMX_ORIENTATION_ORTHOGONAL)
            tileset->object_align = (TMX_ALIGN_BOTTOM | TMX_ALIGN_LEFT);
        else if (context->map->orientation == TMX_ORIENTATION_ISOMETRIC)
            tileset->object_align = TMX_ALIGN_BOTTOM;
    }

    size_t tileIndex     = 0;
    TMXbool isCollection = (TMXbool) tileset->columns == 0;
    tmxInitTilesetTiles(tileset, isCollection);

    if (!tmxXmlMoveToContent(context->xml))
        return tileset;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, WORD_TILE))
            tmxXmlParseTile(context, tileset->tiles, isCollection, tileIndex++);
        else if (STREQL(name, WORD_IMAGE))
            tileset->image = tmxXmlParseImage(context);
        else if (STREQL(name, WORD_PROPERTIES))
        {
            tileset->properties = tmxXmlParseProperties(context);
            tileset->flags |= TMX_FLAG_PROPERTIES;
        }
        else if (STREQL(name, WORD_TILE_OFFSET))
        {
            while (tmxXmlReadAttr(context->xml, &name, &value))
            {
                if (STREQL(name, WORD_X))
                    tileset->offset.x = TMX_STR2INT(value);
                else if (STREQL(name, WORD_Y))
                    tileset->offset.y = TMX_STR2INT(value);
            }
            tmxXmlMoveToContent(context->xml);
        }
        else if (STREQL(name, WORD_GRID))
        {
            while (tmxXmlReadAttr(context->xml, &name, &value))
            {
                if (STREQL(name, WORD_WIDTH))
                    tileset->grid.size.w = TMX_STR2INT(value);
                else if (STREQL(name, WORD_HEIGHT))
                    tileset->grid.size.h = TMX_STR2INT(value);
                if (STREQL(name, WORD_ORIENTATION))
                    tileset->grid.orientation = tmxParseOrientation(value);
            }
            tmxXmlMoveToContent(context->xml);
        }
        else if (STREQL(name, "wangsets") || STREQL(name, "terraintypes") || STREQL(name, "transformations"))
        {
            // Skip the types that are only relevant to the editor, but don't warn (if configured)
            tmxXmlSkipElement(context->xml);
        }
        else
        {
            tmxXmlWarnElement(WORD_TILESET, name);
            tmxXmlSkipElement(context->xml);
        }
    }
    return tileset;
}

static TMXmap *
tmxXmlParseMap(TMXcontext *context)
{
    if (!tmxXmlAssertElement(context->xml, WORD_MAP))
        return NULL;

    TMXmap *map  = TMX_ALLOC(TMXmap);
    context->map = map;

    const char *name;
    const char *value;
    size_t size;

    size_t layerCapa   = 6;
    size_t tilesetCapa = 4;
    map->layers        = tmxCalloc(layerCapa, sizeof(TMXlayer *));
    map->tilesets      = tmxCalloc(tilesetCapa, sizeof(TMXmaptileset));

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_VERSION))
            map->version = tmxStringDup(value);
        else if (STREQL(name, WORD_TILED_VERSION))
            map->tiled_version = tmxStringDup(value);
        else if (STREQL(name, WORD_CLASS))
            map->class = tmxStringDup(value);
        else if (STREQL(name, WORD_ORIENTATION))
            map->orientation = tmxParseOrientation(value);
        else if (STREQL(name, WORD_RENDER_ORDER))
            map->render_order = tmxParseRenderOrder(value);
        else if (STREQL(name, WORD_WIDTH))
            map->size.w = TMX_STR2INT(value);
        else if (STREQL(name, WORD_HEIGHT))
            map->size.h = TMX_STR2INT(value);
        else if (STREQL(name, WORD_TILE_WIDTH))
            map->tile_size.w = TMX_STR2INT(value);
        else if (STREQL(name, WORD_TILE_HEIGHT))
            map->tile_size.h = TMX_STR2INT(value);
        else if (STREQL(name, WORD_HEX_SIDE_LENGTH))
            map->hex_side = TMX_STR2INT(value);
        else if (STREQL(name, WORD_STAGGER_AXIS))
            map->stagger.axis = tmxParseStaggerAxis(value);
        else if (STREQL(name, WORD_STAGGER_INDEX))
            map->stagger.index = tmxParseStaggerIndex(value);
        else if (STREQL(name, WORD_PARALLAX_ORIGIN_X))
            map->parallax_origin.x = TMX_STR2FLT(value);
        else if (STREQL(name, WORD_PARALLAX_ORIGIN_Y))
            map->parallax_origin.y = TMX_STR2FLT(value);
        else if (STREQL(name, WORD_INFINITE))
            map->infinite = TMX_STR2BOOL(value);
        else if (STREQL(name, WORD_BACKGROUND_COLOR))
        {
            map->background_color = tmxParseColor(value);
            map->flags |= TMX_FLAG_COLOR;
        }
#ifdef TMX_WARN_UNHANDLED
        else if (!STREQL(name, WORD_NEXT_LAYER_ID) && !STREQL(name, WORD_NEXT_OBJECT_ID) && !STREQL(name, WORD_COMPRESSION_LEVEL))
            tmxXmlWarnAttribute(WORD_MAP, name);
#endif
    }

    TMXlayer *layer;
    TMXmaptileset mapTileset;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, WORD_PROPERTIES))
        {
            map->properties = tmxXmlParseProperties(context);
            map->flags |= TMX_FLAG_PROPERTIES;
        }
        else if (STREQL(name, WORD_LAYER) || STREQL(name, WORD_OBJECT_GROUP) || STREQL(name, WORD_IMAGE_LAYER) || STREQL(name, WORD_GROUP))
        {
            layer = tmxXmlParseLayer(context, name);
            tmxArrayPush(TMXlayer *, map->layers, layer, map->layer_count, layerCapa);
        }
        else if (STREQL(name, WORD_TILESET))
        {
            mapTileset.tileset = tmxXmlParseTileset(context, &mapTileset.first_gid);
            tmxArrayPush(TMXmaptileset, map->tilesets, mapTileset, map->tileset_count, tilesetCapa);
        }
        else
        {
            tmxXmlWarnElement(WORD_MAP, name);
            tmxXmlSkipElement(context->xml);
        }
    }

    tmxArrayFinish(TMXlayer *, map->layers, map->layer_count, layerCapa);
    tmxArrayFinish(TMXmaptileset, map->tilesets, map->tileset_count, tilesetCapa);

    map->pixel_size = (TMXsize){map->size.w * map->tile_size.w, map->size.h * map->tile_size.h};
    return map;
}

static TMXtemplate *
tmxXmlParseTemplate(TMXcontext *context)
{
    const char *name;
    size_t size;
    TMXtemplate *template = TMX_ALLOC(TMXtemplate);

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, WORD_TILESET))
            template->tileset = tmxXmlParseTileset(context, &template->first_gid);
        else if (STREQL(name, WORD_OBJECT))
            template->object = tmxXmlParseObject(context);
        else
        {
            tmxXmlWarnElement(WORD_TEMPLATE, name);
            tmxXmlSkipElement(context->xml);
        }
    }
    return template;
}

TMXmap *
tmxParseMapXml(TMXcontext *context)
{
    TMXmap *map;
    context->xml = tmxXmlReaderInit(context->text);
    tmxXmlMoveToElement(context->xml, WORD_MAP);
    map = tmxXmlParseMap(context);
    tmxXmlReaderFree(context->xml);
    return map;
}

TMXtileset *
tmxParseTilesetXml(TMXcontext *context)
{
    TMXtileset *tileset;
    context->xml = tmxXmlReaderInit(context->text);
    tmxXmlMoveToElement(context->xml, WORD_TILESET);
    tileset = tmxXmlParseTileset(context, NULL);
    tmxXmlReaderFree(context->xml);
    return tileset;
}

TMXtemplate *
tmxParseTemplateXml(TMXcontext *context)
{
    TMXtemplate *template;
    context->xml = tmxXmlReaderInit(context->text);
    tmxXmlMoveToElement(context->xml, WORD_TEMPLATE);
    template = tmxXmlParseTemplate(context);
    tmxXmlReaderFree(context->xml);
    return template;
}