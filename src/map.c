#include "cwalk.h"
#include "internal.h"
#include "tmx/cache.h"
#include "tmx/compression.h"
#include "tmx/error.h"
#include "tmx/memory.h"
#include "tmx/properties.h"
#include "tmx/xml.h"
#include <stdio.h>
#include <string.h>

#define tmxXmlWarnElement(parent, elemName)   tmxErrorFormat(TMX_ERR_WARN, "Unhandled child element <%s> in <%s>.", elemName, parent)
#define tmxXmlWarnAttribute(parent, attrName) tmxErrorFormat(TMX_ERR_WARN, "Unhandled child attribute \"%s\" in <%s>.", attrName, parent)

TMXtileset *tmxLoadTilesetXml(const char *filename, TMXcache *cache, size_t bufferSize);

typedef struct TMXcontext
{
    TMXcache *cache;
    TMXmap *map;
    char *base_directory;
    TMXxmlreader *xml;
    size_t buffer_size;
    char *text;
    TMXbool freeText;
} TMXcontext;

void
tmxMapFree(TMXmap *map)
{
    if (!map)
        return;

    tmxFree(map);
}

#pragma region Enums

static inline TMXenum
tmxParsePropertyType(const char *value)
{
    if (STREQL(value, "string"))
        return TMX_PROPERTY_STRING;
    if (STREQL(value, "int"))
        return TMX_PROPERTY_INTEGER;
    if (STREQL(value, "float"))
        return TMX_PROPERTY_FLOAT;
    if (STREQL(value, "bool"))
        return TMX_PROPERTY_BOOL;
    if (STREQL(value, "color"))
        return TMX_PROPERTY_COLOR;
    if (STREQL(value, "file"))
        return TMX_PROPERTY_FILE;
    if (STREQL(value, "object"))
        return TMX_PROPERTY_OBJECT;
    if (STREQL(value, "class"))
        return TMX_PROPERTY_CLASS;

    tmxErrorUnknownEnum("property type", value);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseOrientation(const char *value)
{
    if STREQL (value, "orthogonal")
        return TMX_ORIENTATION_ORTHOGONAL;
    if STREQL (value, "isometric")
        return TMX_ORIENTATION_ISOMETRIC;
    if STREQL (value, "staggered")
        return TMX_ORIENTATION_STAGGERED;
    if STREQL (value, "hexagonal")
        return TMX_ORIENTATION_HEXAGONAL;

    tmxErrorUnknownEnum("orientation", value);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseRenderOrder(const char *value)
{
    if (STREQL(value, "right-down"))
        return TMX_RENDER_RIGHT_DOWN;
    if (STREQL(value, "right-up"))
        return TMX_RENDER_RIGHT_UP;
    if (STREQL(value, "left-down"))
        return TMX_RENDER_LEFT_DOWN;
    if (STREQL(value, "left-up"))
        return TMX_RENDER_LEFT_UP;

    tmxErrorUnknownEnum("render order", value);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseStaggerAxis(const char *value)
{
    if (STREQL(value, "x"))
        return TMX_STAGGER_AXIS_X;
    if (STREQL(value, "y"))
        return TMX_STAGGER_AXIS_Y;

    tmxErrorUnknownEnum("stagger axis", value);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseStaggerIndex(const char *value)
{
    if (STREQL(value, "even"))
        return TMX_STAGGER_INDEX_EVEN;
    if (STREQL(value, "odd"))
        return TMX_STAGGER_INDEX_ODD;

    tmxErrorUnknownEnum("stagger index", value);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseLayerType(const char *value, TMXbool infinite)
{
    if (STREQL(value, "layer"))
        return infinite ? TMX_LAYER_CHUNK : TMX_LAYER_TILE;
    if (STREQL(value, "objectgroup"))
        return TMX_LAYER_OBJGROUP;
    if (STREQL(value, "imagelayer"))
        return TMX_LAYER_IMAGE;
    if (STREQL(value, "group"))
        return TMX_LAYER_GROUP;

    // In theory this should be unreachable
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseDrawOrder(const char *value)
{
    if (STREQL(value, "index"))
        return TMX_DRAW_INDEX;
    if (!STREQL(value, "topdown"))
        tmxErrorUnknownEnum("draw order", value);

    return TMX_DRAW_TOPDOWN;
}

static inline TMXflag
tmxParseAlignH(const char *value)
{
    if (STREQL(value, "left"))
        return TMX_ALIGN_LEFT;
    if (STREQL(value, "right"))
        return TMX_ALIGN_RIGHT;
    if (STREQL(value, "center"))
        return TMX_ALIGN_CENTER_H;
    if (STREQL(value, "justify"))
        return TMX_ALIGN_RIGHT;

    tmxErrorUnknownEnum("horizontal align", value);
    return TMX_ALIGN_LEFT;
}

static inline TMXflag
tmxParseAlignV(const char *value)
{
    if (STREQL(value, "top"))
        return TMX_ALIGN_TOP;
    if (STREQL(value, "bottom"))
        return TMX_ALIGN_BOTTOM;
    if (STREQL(value, "center"))
        return TMX_ALIGN_CENTER_V;

    tmxErrorUnknownEnum("vertical align", value);
    return TMX_ALIGN_LEFT;
}

static inline TMXenum
tmxParseObjectAlignment(const char *value)
{
    if (STREQL(value, "topleft"))
        return (TMX_ALIGN_TOP | TMX_ALIGN_LEFT);
    if (STREQL(value, "topright"))
        return (TMX_ALIGN_TOP | TMX_ALIGN_RIGHT);
    if (STREQL(value, "top"))
        return TMX_ALIGN_TOP;

    if (STREQL(value, "bottomleft"))
        return (TMX_ALIGN_BOTTOM | TMX_ALIGN_LEFT);
    if (STREQL(value, "bottomright"))
        return (TMX_ALIGN_BOTTOM | TMX_ALIGN_RIGHT);
    if (STREQL(value, "bottom"))
        return TMX_ALIGN_BOTTOM;

    // Not sure if this is ever an actual value or if it is actually just missing/unspecified...
    if (!STREQL(value, "unspecified"))
        tmxErrorUnknownEnum("object alignment", value);

    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseRenderSize(const char *value)
{
    if (STREQL(value, "tile"))
        return TMX_RENDER_SIZE_TILE;
    if (STREQL(value, "grid"))
        return TMX_RENDER_SIZE_GRID;

    tmxErrorUnknownEnum("render size", value);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseFillMode(const char *value)
{
    if (STREQL(value, "stretch"))
        return TMX_FILL_MODE_STRETCH;
    if (STREQL(value, "preserve-aspect-fit"))
        return TMX_FILL_MODE_PRESERVE;

    tmxErrorUnknownEnum("fill mode", value);
    return TMX_UNSPECIFIED;
}

#pragma endregion

static inline void
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
tmxXmlParseDataType(TMXxmlreader *xml, TMXenum *encoding, TMXenum *compression)
{
    *encoding    = TMX_ENCODING_NONE;
    *compression = TMX_COMPRESSION_NONE;

    const char *name;
    const char *value;

    if (!tmxXmlAssertElement(xml, "data"))
        return;

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
#ifdef TMX_WARN_UNHANDLED
        else
            tmxXmlWarnAttribute("data", name);
#endif
    }
}

static TMXproperties *
tmxXmlParseProperties(TMXcontext *context)
{
    const char *name;
    const char *value;
    size_t size;
    TMXproperties *properties = NULL;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, "property"))
            continue;

        TMXproperties *entry  = tmxCalloc(1, sizeof(TMXproperties));
        TMXproperty *property = &entry->value; // TODO
        while (tmxXmlReadAttr(context->xml, &name, &value))
        {
            if (STREQL(name, "name"))
                property->name = tmxStringDup(value);
            else if (STREQL(name, "type"))
                property->type = tmxParsePropertyType(value);
            else if (STREQL(name, "propertytype"))
                property->custom_type = value;
            else if (STREQL(name, "value"))
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
#ifdef TMX_WARN_UNHANDLED
            else
                tmxXmlWarnAttribute("property", name);
#endif
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
    TMXimage *cached, *image = tmxCalloc(1, sizeof(TMXimage));

    while (tmxXmlReadAttr(context->xml, &name, &value))
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
#ifdef TMX_WARN_UNHANDLED
        else
            tmxXmlWarnAttribute("image", name);
#endif
    }

    if (image->source && tmxCacheTryGetImage(context->cache, image->source, &cached))
    {
        // This is not ideal...
        tmxFree((void *) image->source);
        tmxFree((void *) image->format);
        tmxFree(image);
        return cached;
    }

    if (!tmxXmlMoveToContent(context->xml))
        return image;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, "data"))
            continue;

        image->flags |= TMX_FLAG_EMBEDDED;

        TMXenum encoding, compression;
        tmxXmlParseDataType(context->xml, &encoding, &compression);
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

    tmxImageUserLoad(image, context->base_directory, context->cache);
    return image;
}

static struct TMXtext *
tmxXmlParseObjectText(TMXcontext *context)
{
    struct TMXtext *text = TMX_CALLOC(struct TMXtext);

    const char *name;
    const char *value;
    size_t size;

    // Set defaults
    TMXflag halign   = TMX_ALIGN_LEFT;
    TMXflag valign   = TMX_ALIGN_TOP;
    text->pixel_size = 16;
    text->kerning    = TMX_TRUE;
    text->wrap       = TMX_TRUE;
#ifdef TMX_PACKED_COLOR
    text->color.a = 255;
#else
    text->color.a = 1.0f;
#endif

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, "fontfamily"))
            text->font = tmxStringDup(value);
        else if (STREQL(name, "pixelsize"))
            text->pixel_size = TMX_STR2INT(value);
        else if (STREQL(name, "wrap"))
            text->wrap = TMX_STR2BOOL(value);
        else if (STREQL(name, "color"))
            text->color = tmxParseColor(value);
        else if (STREQL(name, "bold"))
            text->style |= TMX_FONT_STYLE_BOLD;
        else if (STREQL(name, "italic"))
            text->style |= TMX_FONT_STYLE_ITALIC;
        else if (STREQL(name, "underline"))
            text->style |= TMX_FONT_STYLE_UNDERLINE;
        else if (STREQL(name, "strikeout"))
            text->style |= TMX_FONT_STYLE_STRIKEOUT;
        else if (STREQL(name, "kerning"))
            text->kerning = TMX_STR2BOOL(value);
        else if (STREQL(name, "halign"))
            halign = tmxParseAlignH(value);
        else if (STREQL(name, "valign"))
            valign = tmxParseAlignV(value);
#ifdef TMX_WARN_UNHANDLED
        else
            tmxXmlWarnAttribute("text", name);
#endif
    }
    text->align = (halign | valign);

    if (tmxXmlMoveToContent(context->xml))
    {
        if (tmxXmlReadStringContents(context->xml, &value, &size, TMX_FALSE))
            text->string = tmxStringCopy(value, size);
    }

    return text;
}

static void
tmxObjectFromTemplate(TMXobject *dst, TMXobject *src)
{
    // TODO
}

static TMXobject *
tmxXmlParseObject(TMXcontext *context)
{
    TMXobject *object = TMX_CALLOC(TMXobject);

    const char *name;
    const char *value;
    size_t size;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, "id"))
            object->id = TMX_STR2INT(value);
        else if (STREQL(name, "name"))
        {
            object->name = tmxStringDup(value);
            object->flags |= TMX_FLAG_NAME;
        }
        else if (STREQL(name, "type"))
            object->class = tmxStringDup(value);
        else if (STREQL(name, "x"))
        {
            object->position.x = TMX_STR2FLT(value);
            object->flags |= TMX_FLAG_POSITION;
        }
        else if (STREQL(name, "y"))
        {
            object->position.y = TMX_STR2FLT(value);
            object->flags |= TMX_FLAG_POSITION;
        }
        else if (STREQL(name, "width"))
        {
            object->size.x = TMX_STR2FLT(value);
            object->flags |= TMX_FLAG_SIZE;
        }
        else if (STREQL(name, "height"))
        {
            object->size.y = TMX_STR2FLT(value);
            object->flags |= TMX_FLAG_SIZE;
        }
        else if (STREQL(name, "rotation"))
        {
            object->rotation = TMX_STR2FLT(value);
            object->flags |= TMX_FLAG_ROTATION;
        }
        else if (STREQL(name, "gid"))
        {
            char *end   = NULL;
            object->gid = (TMXgid) strtoul(value, &end, 10);
            object->flags |= TMX_FLAG_GID;
        }
        else if (STREQL(name, "visible"))
        {
            object->visible = TMX_STR2BOOL(value);
            object->flags |= TMX_FLAG_VISIBLE;
        }
        else if (STREQL(name, "template"))
        {
            // TODO: Load from template
        }
#ifdef TMX_WARN_UNHANDLED
        else
            tmxXmlWarnAttribute("object", name);
#endif
    }

    // Move to contents. Return early if there is none.
    if (!tmxXmlMoveToContent(context->xml))
        return object;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, "properties"))
        {
            object->properties = tmxXmlParseProperties(context);
            continue;
        }
        else if (STREQL(name, "point"))
        {
            object->type = TMX_OBJECT_POINT;
        }
        else if (STREQL(name, "ellipse"))
        {
            object->type = TMX_OBJECT_ELLIPSE;
        }
        else if (STREQL(name, "polygon") || STREQL(name, "polyline"))
        {
            object->type = STREQL(name, "polygon") ? TMX_OBJECT_POLYGON : TMX_OBJECT_POLYLINE;
            tmxXmlReadAttr(context->xml, &name, &value);
            tmxXmlMoveToContent(context->xml);
            // It is safe to stomp all over this pointer, it is temporary and no longer valid on the next loop
            tmxParsePoints((char *) value, &object->poly);
            object->flags |= TMX_FLAG_POINTS;
        }
        else if (STREQL(name, "text"))
        {
            object->type = TMX_OBJECT_TEXT;
            object->flags |= TMX_FLAG_TEXT;
            object->text = tmxXmlParseObjectText(context);
        }
#if TMX_WARN_UNHANDLED
        else
        {
            tmxXmlWarnElement("object", name);
        }
#endif
        tmxXmlSkipElement(context->xml);
    }

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

            if (!STREQL(str, "tile"))
            {
                tmxXmlSkipElement(context->xml);
#if TMX_WARN_UNHANDLED
                tmxXmlWarnElement("data/chunk", str);
#endif
                continue;
            }

            gid = 0;
            while (tmxXmlReadAttr(context->xml, &str, &value))
            {
                if (STREQL(str, "id"))
                    gid = strtoul(str, &end, 10);
#ifdef TMX_WARN_UNHANDLED
                else
                    tmxXmlWarnAttribute("tile", str);
#endif
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
    tmxXmlParseDataType(context->xml, &encoding, &compression);

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
                if (STREQL(name, "x"))
                    chunk->bounds.x = TMX_STR2INT(value);
                else if (STREQL(name, "y"))
                    chunk->bounds.y = TMX_STR2INT(value);
                else if (STREQL(name, "width"))
                    chunk->bounds.w = TMX_STR2INT(value);
                else if (STREQL(name, "height"))
                    chunk->bounds.h = TMX_STR2INT(value);
#ifdef TMX_WARN_UNHANDLED
                else
                    tmxXmlWarnAttribute("chunk", name);
#endif
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

    TMXlayer *layer = TMX_CALLOC(TMXlayer);
    layer->type     = tmxParseLayerType(layerType, context->map->infinite);

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, "id"))
            layer->id = TMX_STR2INT(value);
        else if (STREQL(name, "name"))
            layer->name = tmxStringDup(value);
        else if (STREQL(name, "class"))
            layer->class = tmxStringDup(value);
        else if (STREQL(name, "x"))
            layer->position.x = TMX_STR2INT(value);
        else if (STREQL(name, "y"))
            layer->position.y = TMX_STR2INT(value);
        else if (STREQL(name, "width"))
            layer->size.w = TMX_STR2INT(value);
        else if (STREQL(name, "height"))
            layer->size.h = TMX_STR2INT(value);
        else if (STREQL(name, "opacity"))
            layer->opacity = TMX_STR2FLT(value);
        else if (STREQL(name, "visible"))
            layer->visible = TMX_STR2BOOL(value);
        else if (STREQL(name, "offsetx"))
            layer->offset.x = TMX_STR2INT(value);
        else if (STREQL(name, "offsety"))
            layer->offset.y = TMX_STR2INT(value);
        else if (STREQL(name, "parallaxx"))
            layer->parallax.x = TMX_STR2FLT(value);
        else if (STREQL(name, "parallaxy"))
            layer->parallax.y = TMX_STR2FLT(value);
        else if (STREQL(name, "tintcolor"))
        {
            layer->tint_color = tmxParseColor(value);
            layer->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, "draworder")) // <objectgroup> only
            layer->draw_order = tmxParseDrawOrder(value);
        else if (STREQL(name, "repeatx")) // <imagelayer> only
            layer->repeat.x = TMX_STR2BOOL(value);
        else if (STREQL(name, "repeaty")) // <imagelayer> only
            layer->repeat.y = TMX_STR2BOOL(value);
#ifdef TMX_WARN_UNHANDLED
        else
            tmxXmlWarnAttribute(layerType, name);
#endif
    }

    if (!tmxXmlMoveToContent(context->xml))
    {
        tmxError(TMX_ERR_PARSE);
        return layer;
    }

    TMXobject *previousObject = NULL;
    TMXlayer *previousGroup   = NULL;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, "properties"))
            layer->properties = tmxXmlParseProperties(context);
        else if (STREQL(name, "data")) // <layer>
        {
            tmxXmlMoveToContent(context->xml);
            tmxXmlParseTileData(context, layer);
        }
        else if (STREQL(name, "object")) // <objectgroup>
        {
            TMXobject *obj = tmxXmlParseObject(context);
            if (!obj)
            {
                tmxErrorMessage(TMX_ERR_PARSE, "Failed to parse map object.");
                continue;
            }

            if (previousObject)
                previousObject->next = obj;
            else
                layer->data.objects = obj;

            layer->count++;
            previousObject = obj;
        }
        else if (STREQL(name, "image")) // <imagelayer>
        {
            layer->data.image = tmxXmlParseImage(context);
        }
        else if (STREQL(name, "layer") || STREQL(name, "objectgroup") || STREQL(name, "imagelayer") || STREQL(name, "group")) // <group>
        {
            TMXlayer *child = tmxXmlParseLayer(context, name);
            if (!child)
                continue;

            if (previousGroup)
                previousGroup->next = child;
            else
                layer->data.group = child;
            previousGroup = child;
            layer->count++;
        }
        else
        {
            tmxXmlSkipElement(context->xml);
#if TMX_WARN_UNHANDLED
            tmxXmlWarnElement(layerType, name);
#endif
        }
    }

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
        if (!STREQL(name, "frame"))
        {
            tmxXmlSkipElement(context->xml);
#if TMX_WARN_UNHANDLED
            tmxXmlWarnElement("map", name);
#endif
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
            if (STREQL(name, "tileid"))
                frame->id = (TMXtid) strtoul(value, &end, 10);
            else if (STREQL(name, "duration"))
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
        if (!STREQL(name, "object"))
        {
            tmxXmlSkipElement(context->xml);
#if TMX_WARN_UNHANDLED
            tmxXmlWarnElement("objectgroup", name);
#endif
            continue;
        }

        if (collision->count >= capacity)
        {
            capacity *= 2;
            collision->objects = tmxRealloc(collision->objects, capacity * sizeof(TMXobject *));
        }

        object = tmxXmlParseObject(context);
        if (!object)
            continue;

        if (collision->count > 0)
            collision->objects[collision->count - 1]->next = object;
        collision->objects[collision->count++] = object;
    }

    if (collision->count < capacity)
        collision->objects = tmxRealloc(collision->objects, capacity * sizeof(TMXobject *));
}

static void
tmxInitTilesetTiles(TMXtileset *tileset, TMXbool isCollection)
{
    if (!tileset->tile_count)
        return;

    tileset->tiles = tmxCalloc(tileset->tile_count, sizeof(TMXtile));

    // A "classic" tileset based on a single image with uniform tiles
    if (!isCollection)
    {
        size_t i;
        for (i = 0; i < tileset->tile_count; i++)
        {
            tileset->tiles[i].id            = (TMXtid) i;
            tileset->tiles[i].rect.position = (TMXpoint){i % tileset->columns, i / tileset->columns};
            tileset->tiles[i].rect.size     = tileset->tile_size;
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
        if (STREQL(name, "id"))
        {
            TMXtid id = (TMXtid) strtoul(value, &end, 10);
            if (!isCollection)
                tileIndex = id;
            tile     = &tiles[tileIndex];
            tile->id = id;
        }
        else if (STREQL(name, "type"))
            tile->class = tmxStringDup(value);
        else if (STREQL(name, "x"))
            tile->rect.x = TMX_STR2INT(value);
        else if (STREQL(name, "y"))
            tile->rect.y = TMX_STR2INT(value);
        else if (STREQL(name, "width"))
            tile->rect.w = TMX_STR2INT(value);
        else if (STREQL(name, "height"))
            tile->rect.h = TMX_STR2INT(value);
#ifdef TMX_WARN_UNHANDLED
        else
            tmxXmlWarnAttribute("tile", name);
#endif
    }

    if (!tmxXmlMoveToContent(context->xml))
        return;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, "animation"))
            tmxParseAnimation(context, &tile->animation);
        else if (STREQL(name, "objectgroup"))
            tmxParseCollision(context, &tile->collision);
        else if (STREQL(name, "image"))
            tile->image = tmxXmlParseImage(context);
        else if (STREQL(name, "properties"))
            tile->properties = tmxXmlParseProperties(context);
        else
        {
            tmxXmlSkipElement(context->xml);
#if TMX_WARN_UNHANDLED
            tmxXmlWarnElement("tile", name);
#endif
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
        if (STREQL(name, "firstgid"))
        {
            if (firstGid)
                *firstGid = strtoul(value, &end, 10);
            continue;
        }
        else if (STREQL(name, "source"))
        {
            char buffer[TMX_MAX_PATH];
            tmxPathResolve(value, context->base_directory, buffer, TMX_MAX_PATH);
            tmxXmlMoveToContent(context->xml);
            if (tmxCacheTryGetTileset(context->cache, buffer, &tileset))
                return tileset;

            tileset = tmxLoadTilesetXml(buffer, context->cache, context->buffer_size);
            return tileset;
        }

        if (!tileset)
            tileset = TMX_CALLOC(TMXtileset);

        // The "firstgid" and "source" would be first/only attributes in an external tileset, so if neither are defined by
        // this point, then this is the actual tileset definition, and it needs allocated before reading anything more.
        //
        // Ideally, this would be far easier if they were differentiated with separate <tileset> and <tilesetref> elements.

        if (STREQL(name, "name"))
            tileset->name = tmxStringDup(value);
        else if (STREQL(name, "class"))
            tileset->class = tmxStringDup(value);
        else if (STREQL(name, "tilewidth"))
            tileset->tile_size.w = TMX_STR2INT(value);
        else if (STREQL(name, "tileheight"))
            tileset->tile_size.h = TMX_STR2INT(value);
        else if (STREQL(name, "spacing"))
            tileset->spacing = TMX_STR2INT(value);
        else if (STREQL(name, "margin"))
            tileset->margin = TMX_STR2INT(value);
        else if (STREQL(name, "tilecount"))
            tileset->tile_count = strtoul(value, &end, 10);
        else if (STREQL(name, "columns"))
            tileset->columns = TMX_STR2INT(value);
        else if (STREQL(name, "objectalignment"))
            tileset->object_align = tmxParseObjectAlignment(value);
        else if (STREQL(name, "tilerendersize"))
            tileset->render_size = tmxParseRenderSize(value);
        else if (STREQL(name, "fillmode"))
            tileset->fill_mode = tmxParseFillMode(value);
        else if (STREQL(name, "version"))
            tileset->version = tmxStringDup(value);
        else if (STREQL(name, "tiledversion"))
            tileset->tiled_version = tmxStringDup(value);
#ifdef TMX_WARN_UNHANDLED
        else
            tmxXmlWarnAttribute("tileset", name);
#endif
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
        if (STREQL(name, "tile"))
            tmxXmlParseTile(context, tileset->tiles, isCollection, tileIndex++);
        else if (STREQL(name, "image"))
            tileset->image = tmxXmlParseImage(context);
        else if (STREQL(name, "properties"))
            tileset->properties = tmxXmlParseProperties(context);
        else if (STREQL(name, "tileoffset"))
        {
            while (tmxXmlReadAttr(context->xml, &name, &value))
            {
                if (STREQL(name, "x"))
                    tileset->offset.x = TMX_STR2INT(value);
                else if (STREQL(name, "y"))
                    tileset->offset.y = TMX_STR2INT(value);
            }
            tmxXmlMoveToContent(context->xml);
        }
        else if (STREQL(name, "grid"))
        {
            while (tmxXmlReadAttr(context->xml, &name, &value))
            {
                if (STREQL(name, "width"))
                    tileset->grid.size.w = TMX_STR2INT(value);
                else if (STREQL(name, "height"))
                    tileset->grid.size.h = TMX_STR2INT(value);
                if (STREQL(name, "orientation"))
                    tileset->grid.orientation = tmxParseOrientation(value);
            }
            tmxXmlMoveToContent(context->xml);
        }
        else if (STREQL(name, "wangsets"))
        {
            tmxXmlSkipElement(context->xml); // TODO
        }
        else if (STREQL(name, "terraintypes"))
        {
            tmxXmlSkipElement(context->xml); // TODO
        }
        else if (STREQL(name, "transformations"))
        {
            tmxXmlSkipElement(context->xml); // TODO
        }
        else
        {
            tmxXmlSkipElement(context->xml);
#if TMX_WARN_UNHANDLED
            tmxXmlWarnElement("tileset", name);
#endif
        }
    }

    return tileset;
}

static TMXmap *
tmxXmlParseMap(TMXcontext *context)
{
    if (!tmxXmlAssertElement(context->xml, "map"))
        return NULL;

    TMXmap *map  = TMX_CALLOC(TMXmap);
    context->map = map;

    const char *name;
    const char *value;
    size_t size;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, "version"))
            map->version = tmxStringDup(value);
        else if (STREQL(name, "tiledversion"))
            map->tiled_version = tmxStringDup(value);
        else if (STREQL(name, "class"))
            map->class = tmxStringDup(value);
        else if (STREQL(name, "orientation"))
            map->orientation = tmxParseOrientation(value);
        else if (STREQL(name, "renderorder"))
            map->render_order = tmxParseRenderOrder(value);
        else if (STREQL(name, "width"))
            map->size.w = TMX_STR2INT(value);
        else if (STREQL(name, "height"))
            map->size.h = TMX_STR2INT(value);
        else if (STREQL(name, "tilewidth"))
            map->tile_size.w = TMX_STR2INT(value);
        else if (STREQL(name, "tileheight"))
            map->tile_size.h = TMX_STR2INT(value);
        else if (STREQL(name, "hexsidelength"))
            map->hex_side = TMX_STR2INT(value);
        else if (STREQL(name, "staggeraxis"))
            map->stagger.axis = tmxParseStaggerAxis(value);
        else if (STREQL(name, "staggerindex"))
            map->stagger.index = tmxParseStaggerIndex(value);
        else if (STREQL(name, "parallaxoriginx"))
            map->parallax_origin.x = TMX_STR2FLT(value);
        else if (STREQL(name, "parallaxoriginy"))
            map->parallax_origin.y = TMX_STR2FLT(value);
        else if (STREQL(name, "infinite"))
            map->infinite = TMX_STR2BOOL(value);
        else if (STREQL(name, "backgroundcolor"))
        {
            map->background_color = tmxParseColor(value);
            map->flags |= TMX_FLAG_COLOR;
        }
#ifdef TMX_WARN_UNHANDLED
        else if (!STREQL(name, "nextlayerid") && !STREQL(name, "nextobjectid"))
            tmxXmlWarnAttribute("map", name);
#endif
    }

    TMXlayer *layer, *previousLayer          = NULL;
    TMXmaptileset *tileset, *previousTileset = NULL;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, "properties"))
            map->properties = tmxXmlParseProperties(context);
        else if (STREQL(name, "layer") || STREQL(name, "objectgroup") || STREQL(name, "imagelayer") || STREQL(name, "group"))
        {
            layer = tmxXmlParseLayer(context, name);
            if (!layer)
            {
                tmxErrorFormat(TMX_ERR_PARSE, "Failed to parse %s layer", name);
                continue;
            }
            if (previousLayer)
                previousLayer->next = layer;
            else
                map->layers = layer;
            previousLayer = layer;
            map->layer_count++;
        }
        else if (STREQL(name, "tileset"))
        {
            TMXgid firstGid;
            TMXtileset *ts = tmxXmlParseTileset(context, &firstGid);
            if (ts)
            {
                tileset            = TMX_CALLOC(TMXmaptileset);
                tileset->first_gid = firstGid;
                tileset->tileset   = ts;

                if (previousTileset)
                    previousTileset->next = tileset;
                else
                    map->tilesets = tileset;
                previousTileset = tileset;
                map->tileset_count++;
            }
        }
        else
        {
#if TMX_WARN_UNHANDLED
            tmxXmlWarnElement("map", name);
#endif
            tmxXmlSkipElement(context->xml);
        }
    }

    map->pixel_size = (TMXsize){map->size.w * map->tile_size.w, map->size.h * map->tile_size.h};
    return map;
}

static TMXtemplate *
tmxXmlParseTemplate(TMXcontext *context)
{
    return NULL; // TODO
}

static TMXbool
tmxContextInit(TMXcontext *context, const char *filename, const char *text, TMXcache *cache, size_t bufferSize)
{
    if (!context)
        return TMX_FALSE;

    memset(context, 0, sizeof(TMXcontext));
    context->cache       = cache;
    context->buffer_size = bufferSize;

    if (filename)
    {
        size_t dirLen;
        cwk_path_get_dirname(filename, &dirLen);
        if (dirLen)
            context->base_directory = tmxStringCopy(filename, dirLen);
        
        context->text = tmxFileReadAll(filename, NULL);
        context->freeText = TMX_TRUE;
    }
    else
    {
        context->text = (char*) text;
        context->freeText = TMX_FALSE;
    }

    TMX_ASSERT(context->text != NULL);
    context->xml = tmxXmlReaderInit(context->text, bufferSize);

    if (context->xml)
        return TMX_TRUE;

    tmxErrorMessage(TMX_ERR_PARSE, "Failed to create parsing context.");
    return TMX_FALSE;
}

static inline tmxContextDeinit(TMXcontext *context)
{
    tmxXmlReaderFree(context->xml);
    if (context->base_directory)
        tmxFree(context->base_directory);
    if (context->freeText)
        tmxFree(context->text);
}

static
TMXtileset *tmxParseTilesetImpl(const char *text, TMXcache *cache, size_t bufferSize, const char *filename)
{
    TMXtileset *tileset = NULL;
    if (tmxCacheTryGetTileset(cache, filename, &tileset))
        return tileset;

    TMXcontext context;
    if (!tmxContextInit(&context, filename, NULL, cache, bufferSize))
        return NULL;
    tmxXmlMoveToElement(context.xml, "tileset");

    tileset = tmxXmlParseTileset(&context, NULL);
    tmxContextDeinit(&context);

    tmxCacheAddTileset(cache, filename, tileset);
    return tileset;
}

static
TMXmap *tmxParseMapImpl(const char *text, TMXcache *cache, size_t bufferSize, const char *filename)
{
    TMXcontext context;
    if (!tmxContextInit(&context, filename, NULL, cache, bufferSize))
        return NULL;

    tmxXmlMoveToElement(context.xml, "map");
    TMXmap *map = tmxXmlParseMap(&context);
    tmxContextDeinit(&context);
    return map;
}

static
TMXtemplate *tmxParseTemplateImpl(const char *text, TMXcache *cache, size_t bufferSize, const char *filename)
{
    TMXtemplate *template;
    if (tmxCacheTryGetTemplate(cache, filename, &template))
        return template;

    TMXcontext context;
    if (!tmxContextInit(&context, filename, NULL, cache, bufferSize))
        return NULL;

    tmxXmlMoveToElement(context.xml, "template");
    template = tmxXmlParseTemplate(&context);
    tmxContextDeinit(&context);

    tmxCacheAddTemplate(cache, filename, template);
    return template;
}


TMXtileset *tmxParseTileset(const char *text, TMXcache *cache, size_t bufferSize)
{
    return tmxParseTilesetImpl(text, cache, bufferSize, NULL);
}

TMXmap *tmxParseMap(const char *text, TMXcache *cache, size_t bufferSize)
{
    return tmxParseMapImpl(text, cache, bufferSize, NULL);
}

TMXtemplate *tmxParseTemplate(const char *text, TMXcache *cache, size_t bufferSize)
{
    return tmxParseTemplateImpl(text, cache, bufferSize, NULL);
}

TMXtileset *
tmxLoadTilesetXml(const char *filename, TMXcache *cache, size_t bufferSize)
{
    return tmxParseTilesetImpl(NULL, cache, bufferSize, filename);
}

TMXmap *
tmxLoadMapXml(const char *filename, TMXcache *cache, size_t bufferSize)
{
    return tmxParseMapImpl(NULL, cache, bufferSize, filename);
}

TMXtemplate *
tmxLoadTemplateXml(const char *filename, TMXcache *cache, size_t bufferSize)
{
    return tmxParseTemplateImpl(NULL, cache, bufferSize, filename);
}