#include "cwalk.h"
#include "internal.h"
#include "parse.h"
#include "tmx/compression.h"
#include "internal.h"
#include "tmx/xml.h"
#include <stdio.h>


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
tmxXmlParseDataType(TMXcontext *context, TMX_ENCODING *encoding, TMX_COMPRESSION *compression)
{
    *encoding    = TMX_ENCODING_NONE;
    *compression = TMX_COMPRESSION_NONE;

    const char *name;
    const char *value;

    if (!tmxXmlAssertElement(context->xml, TMX_WORD_DATA))
        return;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, TMX_WORD_ENCODING))
            *encoding = tmxParseEncoding(value);
        else if (STREQL(name, TMX_WORD_COMPRESSION))
            *compression = tmxParseCompression(value);
        else
        {
            tmxXmlWarnAttribute(TMX_WORD_DATA, name);
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
        if (!STREQL(name, TMX_WORD_PROPERTY))
            continue;

        entry    = tmxCalloc(1, sizeof(TMXproperties));
        property = &entry->value;
        while (tmxXmlReadAttr(context->xml, &name, &value))
        {
            if (STREQL(name, TMX_WORD_NAME))
                property->name = tmxStringDup(value);
            else if (STREQL(name, TMX_WORD_TYPE))
                property->type = tmxParsePropertyType(value);
            else if (STREQL(name, WORD_PROPERTY_TYPE))
                property->class = value;
            else if (STREQL(name, TMX_WORD_VALUE))
            {
                // The type is always defined before the value.
                switch (property->type)
                {
                    case TMX_PROPERTY_UNSPECIFIED:
                    case TMX_PROPERTY_FILE:
                    case TMX_PROPERTY_STRING: property->value.string = tmxStringDup(value); break;
                    case TMX_PROPERTY_INTEGER:
                    case TMX_PROPERTY_OBJECT: property->value.integer = tmxParseInt(value); break;
                    case TMX_PROPERTY_FLOAT: property->value.decimal = tmxParseFloat(value); break;
                    case TMX_PROPERTY_BOOL: property->value.integer = tmxParseBool(value); break;
                    case TMX_PROPERTY_COLOR: property->value.color = tmxParseColor(value); break;
                    case TMX_PROPERTY_CLASS: break;
                }
            }
            else
            {
                tmxXmlWarnAttribute(TMX_WORD_PROPERTY, name);
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
        if (STREQL(name, TMX_WORD_FORMAT))
            image->format = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_SOURCE))
        {
            image->source = tmxStringDup(value);
            image->flags |= TMX_FLAG_EXTERNAL;
        }
        else if (STREQL(name, TMX_WORD_TRANS))
        {
            image->transparent = tmxParseColor(value);
            image->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, TMX_WORD_WIDTH))
            image->size.w = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_HEIGHT))
            image->size.h = tmxParseInt(value);
        else
        {
            tmxXmlWarnAttribute(TMX_WORD_IMAGE, name);
        }
    }

    if (!tmxXmlMoveToContent(context->xml))
        return image;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, TMX_WORD_DATA))
            continue;

        image->flags |= TMX_FLAG_EMBEDDED;

        TMX_ENCODING encoding;
        TMX_COMPRESSION compression;
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
    TMX_ALIGN halign   = TMX_ALIGN_LEFT;
    TMX_ALIGN valign   = TMX_ALIGN_TOP;
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
            text->pixel_size = tmxParseInt(value);
            obj->flags |= TMX_FLAG_FONT_SIZE;
        }
        else if (STREQL(name, TMX_WORD_WRAP))
        {
            text->wrap = tmxParseBool(value);
            obj->flags |= TMX_FLAG_WORD_WRAP;
        }
        else if (STREQL(name, TMX_WORD_COLOR))
        {
            text->color = tmxParseColor(value);
            obj->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, TMX_WORD_BOLD))
        {
            if (tmxParseBool(value))
                text->style |= TMX_FONT_STYLE_BOLD;
            else
                text->style &= ~TMX_FONT_STYLE_BOLD;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_BOLD);
        }
        else if (STREQL(name, TMX_WORD_ITALIC))
        {
            if (tmxParseBool(value))
                text->style |= TMX_FONT_STYLE_ITALIC;
            else
                text->style &= ~TMX_FONT_STYLE_ITALIC;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_ITALIC);
        }
        else if (STREQL(name, TMX_WORD_UNDERLINE))
        {
            if (tmxParseBool(value))
                text->style |= TMX_FONT_STYLE_UNDERLINE;
            else
                text->style &= ~TMX_FONT_STYLE_UNDERLINE;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_UNDERLINE);
        }
        else if (STREQL(name, TMX_WORD_STRIKEOUT))
        {
            if (tmxParseBool(value))
                text->style |= TMX_FONT_STYLE_STRIKEOUT;
            else
                text->style &= ~TMX_FONT_STYLE_STRIKEOUT;
            obj->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_STRIKEOUT);
        }
        else if (STREQL(name, TMX_WORD_KERNING))
        {
            text->kerning = tmxParseBool(value);
            obj->flags |= TMX_FLAG_FONT_KERNING;
        }
        else if (STREQL(name, TMX_WORD_HALIGN))
        {
            halign = tmxParseAlignH(value);
            obj->flags |= (TMX_FLAG_ALIGN | TMX_FLAG_HALIGN);
        }
        else if (STREQL(name, TMX_WORD_VALIGN))
        {
            valign = tmxParseAlignV(value);
            obj->flags |= (TMX_FLAG_ALIGN | TMX_FLAG_VALIGN);
        }
        else
        {
            tmxXmlWarnAttribute(TMX_WORD_TEXT, name);
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
        if (STREQL(name, TMX_WORD_ID))
            object->id = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_NAME))
        {
            object->name = tmxStringDup(value);
            object->flags |= TMX_FLAG_NAME;
        }
        else if (STREQL(name, TMX_WORD_TYPE))
            object->class = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_X))
        {
            object->position.x = tmxParseFloat(value);
            object->flags |= (TMX_FLAG_POSITION | TMX_FLAG_X);
        }
        else if (STREQL(name, TMX_WORD_Y))
        {
            object->position.y = tmxParseFloat(value);
            object->flags |= (TMX_FLAG_POSITION | TMX_FLAG_Y);
        }
        else if (STREQL(name, TMX_WORD_WIDTH))
        {
            object->size.x = tmxParseFloat(value);
            object->flags |= (TMX_FLAG_SIZE | TMX_FLAG_WIDTH);
        }
        else if (STREQL(name, TMX_WORD_HEIGHT))
        {
            object->size.y = tmxParseFloat(value);
            object->flags |= (TMX_FLAG_SIZE | TMX_FLAG_HEIGHT);
        }
        else if (STREQL(name, TMX_WORD_ROTATION))
        {
            object->rotation = tmxParseFloat(value);
            object->flags |= TMX_FLAG_ROTATION;
        }
        else if (STREQL(name, TMX_WORD_GID))
        {
            object->gid = (TMXgid) tmxParseUint(value);
            object->flags |= TMX_FLAG_GID;
        }
        else if (STREQL(name, TMX_WORD_VISIBLE))
        {
            object->visible = tmxParseBool(value);
            object->flags |= TMX_FLAG_VISIBLE;
        }
        else if (STREQL(name, TMX_WORD_TYPE) || STREQL(name, TMX_WORD_CLASS))
        {
            object->flags |= TMX_FLAG_CLASS;
            object->class = tmxStringDup(value);
        }
        else if (STREQL(name, TMX_WORD_TEMPLATE))
        {
            char templatePath[TMX_MAX_PATH];
            tmxFileAbsolutePath(value, context->basePath, templatePath, TMX_MAX_PATH);
            object->template = tmxLoadTemplate(templatePath, context->cache, TMX_FORMAT_AUTO);
        }
        else
        {
            tmxXmlWarnAttribute(TMX_WORD_OBJECT, name);
        }
    }

    // Move to contents. Return early if there is none.
    if (!tmxXmlMoveToContent(context->xml))
        return object;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, TMX_WORD_PROPERTIES))
        {
            object->properties = tmxXmlParseProperties(context);
            object->flags |= TMX_FLAG_PROPERTIES;
            continue;
        }
        else if (STREQL(name, TMX_WORD_POINT))
        {
            object->type = TMX_OBJECT_POINT;
        }
        else if (STREQL(name, TMX_WORD_ELLIPSE))
        {
            object->type = TMX_OBJECT_ELLIPSE;
        }
        else if (STREQL(name, TMX_WORD_POLYGON) || STREQL(name, TMX_WORD_POLYLINE))
        {
            object->type = STREQL(name, TMX_WORD_POLYGON) ? TMX_OBJECT_POLYGON : TMX_OBJECT_POLYLINE;
            tmxXmlReadAttr(context->xml, &name, &value);
            tmxXmlMoveToContent(context->xml);
            // It is safe to stomp all over this pointer, it is temporary and no longer valid on the next loop
            tmxParsePoints((char *) value, &object->poly);
            object->flags |= TMX_FLAG_POINTS;
        }
        else if (STREQL(name, TMX_WORD_TEXT))
        {
            object->type = TMX_OBJECT_TEXT;
            object->text = tmxXmlParseObjectText(context, object);
        }
        else
        {
            tmxXmlWarnElement(TMX_WORD_OBJECT, name);
            tmxXmlSkipElement(context->xml);
        }
    }

    if (object->template && object->template->object)
        tmxObjectMergeTemplate(object, object->template->object);

    return object;
}

static void
tmxXmlParseTileIds(TMXcontext *context, TMX_ENCODING encoding, TMX_COMPRESSION compression, TMXgid *output, size_t outputCount)
{
    const char *str;
    size_t strSize;
    tmxXmlMoveToContent(context->xml);

    if (encoding == TMX_ENCODING_NONE)
    {
        size_t i = 0;
        TMXgid gid;
        const char *value;

        while (tmxXmlReadElement(context->xml, &str, &strSize))
        {

            if (!STREQL(str, TMX_WORD_TILE))
            {
                tmxXmlWarnElement("data/chunk", str);
                tmxXmlSkipElement(context->xml);
                continue;
            }

            gid = 0;
            while (tmxXmlReadAttr(context->xml, &str, &value))
            {
                if (STREQL(str, TMX_WORD_ID))
                {
                    gid = tmxParseUint(str);
                }
                else
                {
                    tmxXmlWarnAttribute(TMX_WORD_TILE, str);
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
    TMX_ENCODING encoding;
    TMX_COMPRESSION compression;

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
                if (STREQL(name, TMX_WORD_X))
                    chunk->bounds.x = tmxParseInt(value);
                else if (STREQL(name, TMX_WORD_Y))
                    chunk->bounds.y = tmxParseInt(value);
                else if (STREQL(name, TMX_WORD_WIDTH))
                    chunk->bounds.w = tmxParseInt(value);
                else if (STREQL(name, TMX_WORD_HEIGHT))
                    chunk->bounds.h = tmxParseInt(value);
                else
                {
                    tmxXmlWarnAttribute(TMX_WORD_CHUNK, name);
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
        if (STREQL(name, TMX_WORD_ID))
            layer->id = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_NAME))
            layer->name = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_CLASS))
            layer->class = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_X))
            layer->position.x = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_Y))
            layer->position.y = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_WIDTH))
            layer->size.w = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_HEIGHT))
            layer->size.h = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_OPACITY))
            layer->opacity = tmxParseFloat(value);
        else if (STREQL(name, TMX_WORD_VISIBLE))
            layer->visible = tmxParseBool(value);
        else if (STREQL(name, WORD_OFFSET_X))
            layer->offset.x = tmxParseInt(value);
        else if (STREQL(name, WORD_OFFSET_Y))
            layer->offset.y = tmxParseInt(value);
        else if (STREQL(name, WORD_PARALLAX_X))
            layer->parallax.x = tmxParseFloat(value);
        else if (STREQL(name, WORD_PARALLAX_Y))
            layer->parallax.y = tmxParseFloat(value);
        else if (STREQL(name, WORD_TINT_COLOR))
        {
            layer->tint_color = tmxParseColor(value);
            layer->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, WORD_DRAW_ORDER)) // <objectgroup> only
            layer->draw_order = tmxParseDrawOrder(value);
        else if (STREQL(name, WORD_REPEAT_X)) // <imagelayer> only
            layer->repeat.x = tmxParseBool(value);
        else if (STREQL(name, WORD_REPEAT_Y)) // <imagelayer> only
            layer->repeat.y = tmxParseBool(value);
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
        if (STREQL(name, TMX_WORD_PROPERTIES))
        {
            layer->properties = tmxXmlParseProperties(context);
            layer->flags |= TMX_FLAG_PROPERTIES;
        }
        else if (STREQL(name, TMX_WORD_DATA)) // <layer>
        {
            tmxXmlMoveToContent(context->xml);
            tmxXmlParseTileData(context, layer);
        }
        else if (STREQL(name, TMX_WORD_OBJECT)) // <objectgroup>
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
        else if (STREQL(name, TMX_WORD_IMAGE)) // <imagelayer>
        {
            layer->data.image = tmxXmlParseImage(context);
        }
        else if (STREQL(name, TMX_WORD_LAYER) || STREQL(name, WORD_OBJECT_GROUP) || STREQL(name, WORD_IMAGE_LAYER) ||
                 STREQL(name, TMX_WORD_GROUP)) // <group>
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
    size_t size;
    size_t capacity   = 8;
    animation->frames = tmxMalloc(capacity * sizeof(TMXframe));

    tmxXmlMoveToContent(context->xml);
    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (!STREQL(name, TMX_WORD_FRAME))
        {
            tmxXmlWarnElement(TMX_WORD_MAP, name);
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
                frame->id = (TMXtid) tmxParseUint(value);
            else if (STREQL(name, TMX_WORD_DURATION))
                frame->duration = tmxParseUint(value);
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
        if (!STREQL(name, TMX_WORD_OBJECT))
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
tmxXmlParseTile(TMXcontext *context, TMXtile *tiles, TMX_BOOL isCollection, size_t tileIndex)
{
    const char *name;
    const char *value;
    size_t size;

    TMXtile *tile = NULL;

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, TMX_WORD_ID))
        {
            TMXtid id = (TMXtid) tmxParseUint(value);
            if (!isCollection)
                tileIndex = id;
            tile     = &tiles[tileIndex];
            tile->id = id;
        }
        else if (STREQL(name, TMX_WORD_TYPE))
            tile->class = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_X))
            tile->rect.x = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_Y))
            tile->rect.y = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_WIDTH))
            tile->rect.w = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_HEIGHT))
            tile->rect.h = tmxParseInt(value);
        else
        {
            tmxXmlWarnAttribute(TMX_WORD_TILE, name);
        }
    }

    if (!tmxXmlMoveToContent(context->xml))
        return;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, TMX_WORD_ANIMATION))
            tmxParseAnimation(context, &tile->animation);
        else if (STREQL(name, WORD_OBJECT_GROUP))
            tmxParseCollision(context, &tile->collision);
        else if (STREQL(name, TMX_WORD_IMAGE))
            tile->image = tmxXmlParseImage(context);
        else if (STREQL(name, TMX_WORD_PROPERTIES))
            tile->properties = tmxXmlParseProperties(context);
        else
        {
            tmxXmlWarnElement(TMX_WORD_TILE, name);
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
    size_t size;

    // The XML spec for external/embedded tilesets combined with a forward-only parser makes this a bit wonky.

    while (tmxXmlReadAttr(context->xml, &name, &value))
    {
        if (STREQL(name, WORD_FIRST_GID))
        {
            if (firstGid)
                *firstGid = tmxParseUint(value);
            continue;
        }
        else if (STREQL(name, TMX_WORD_SOURCE))
        {
            char buffer[TMX_MAX_PATH];
            tmxFileAbsolutePath(value, context->basePath, buffer, TMX_MAX_PATH);
            tmxXmlMoveToContent(context->xml);
            if (tmxCacheTryGetTileset(context->cache, buffer, &tileset))
                return tileset;

            tileset = tmxLoadTileset(buffer, context->cache, TMX_FORMAT_AUTO);
            if (tileset)
                tileset->flags |= TMX_FLAG_EXTERNAL;
            return tileset;
        }

        if (!tileset)
            tileset = TMX_ALLOC(TMXtileset);

        // The "firstgid" and "source" would be first/only attributes in an external tileset, so if neither are defined by
        // this point, then this is the actual tileset definition, and it needs allocated before reading anything more.

        if (STREQL(name, TMX_WORD_NAME))
            tileset->name = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_CLASS))
            tileset->class = tmxStringDup(value);
        else if (STREQL(name, WORD_TILE_WIDTH))
            tileset->tile_size.w = tmxParseInt(value);
        else if (STREQL(name, WORD_TILE_HEIGHT))
            tileset->tile_size.h = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_SPACING))
            tileset->spacing = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_MARGIN))
            tileset->margin = tmxParseInt(value);
        else if (STREQL(name, WORD_TILE_COUNT))
            tileset->tile_count = tmxParseUint(value);
        else if (STREQL(name, TMX_WORD_COLUMNS))
            tileset->columns = tmxParseInt(value);
        else if (STREQL(name, WORD_OBJECT_ALIGN))
            tileset->object_align = tmxParseObjectAlignment(value);
        else if (STREQL(name, WORD_TILE_RENDER_SIZE))
            tileset->render_size = tmxParseRenderSize(value);
        else if (STREQL(name, WORD_FILL_MODE))
            tileset->fill_mode = tmxParseFillMode(value);
        else if (STREQL(name, TMX_WORD_VERSION))
            tileset->version = tmxStringDup(value);
        else if (STREQL(name, WORD_TILED_VERSION))
            tileset->tiled_version = tmxStringDup(value);
        else if (STREQL(name, WORD_BACKGROUND_COLOR))
        {
            tileset->background_color = tmxParseColor(value);
            tileset->flags |= TMX_FLAG_COLOR;
        }
        else
        {
            tmxXmlWarnAttribute(TMX_WORD_TILESET, name);
        }
    }

    size_t tileIndex     = 0;
    TMX_BOOL isCollection = (TMX_BOOL) tileset->columns == 0;
    tmxInitTilesetTiles(tileset, isCollection);

    if (!tmxXmlMoveToContent(context->xml))
        return tileset;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, TMX_WORD_TILE))
            tmxXmlParseTile(context, tileset->tiles, isCollection, tileIndex++);
        else if (STREQL(name, TMX_WORD_IMAGE))
            tileset->image = tmxXmlParseImage(context);
        else if (STREQL(name, TMX_WORD_PROPERTIES))
        {
            tileset->properties = tmxXmlParseProperties(context);
            tileset->flags |= TMX_FLAG_PROPERTIES;
        }
        else if (STREQL(name, WORD_TILE_OFFSET))
        {
            while (tmxXmlReadAttr(context->xml, &name, &value))
            {
                if (STREQL(name, TMX_WORD_X))
                    tileset->offset.x = tmxParseInt(value);
                else if (STREQL(name, TMX_WORD_Y))
                    tileset->offset.y = tmxParseInt(value);
            }
            tmxXmlMoveToContent(context->xml);
        }
        else if (STREQL(name, TMX_WORD_GRID))
        {
            while (tmxXmlReadAttr(context->xml, &name, &value))
            {
                if (STREQL(name, TMX_WORD_WIDTH))
                    tileset->grid.size.w = tmxParseInt(value);
                else if (STREQL(name, TMX_WORD_HEIGHT))
                    tileset->grid.size.h = tmxParseInt(value);
                if (STREQL(name, TMX_WORD_ORIENTATION))
                    tileset->grid.orientation = tmxParseOrientation(value);
            }
            tmxXmlMoveToContent(context->xml);
        }
        else if (STREQL(name, TMX_WORD_WANGSETS) || STREQL(name, WORD_TERRAIN_TYPES) || STREQL(name, TMX_WORD_TRANSFORMATIONS))
        {
            // Skip the types that are only relevant to the editor, but don't warn (if configured)
            tmxXmlSkipElement(context->xml);
        }
        else
        {
            tmxXmlWarnElement(TMX_WORD_TILESET, name);
            tmxXmlSkipElement(context->xml);
        }
    }
    return tileset;
}

static TMXmap *
tmxXmlParseMap(TMXcontext *context)
{
    if (!tmxXmlAssertElement(context->xml, TMX_WORD_MAP))
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
        if (STREQL(name, TMX_WORD_VERSION))
            map->version = tmxStringDup(value);
        else if (STREQL(name, WORD_TILED_VERSION))
            map->tiled_version = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_CLASS))
            map->class = tmxStringDup(value);
        else if (STREQL(name, TMX_WORD_ORIENTATION))
            map->orientation = tmxParseOrientation(value);
        else if (STREQL(name, WORD_RENDER_ORDER))
            map->render_order = tmxParseRenderOrder(value);
        else if (STREQL(name, TMX_WORD_WIDTH))
            map->size.w = tmxParseInt(value);
        else if (STREQL(name, TMX_WORD_HEIGHT))
            map->size.h = tmxParseInt(value);
        else if (STREQL(name, WORD_TILE_WIDTH))
            map->tile_size.w = tmxParseInt(value);
        else if (STREQL(name, WORD_TILE_HEIGHT))
            map->tile_size.h = tmxParseInt(value);
        else if (STREQL(name, WORD_HEX_SIDE_LENGTH))
            map->hex_side = tmxParseInt(value);
        else if (STREQL(name, WORD_STAGGER_AXIS))
            map->stagger.axis = tmxParseStaggerAxis(value);
        else if (STREQL(name, WORD_STAGGER_INDEX))
            map->stagger.index = tmxParseStaggerIndex(value);
        else if (STREQL(name, WORD_PARALLAX_ORIGIN_X))
            map->parallax_origin.x = tmxParseFloat(value);
        else if (STREQL(name, WORD_PARALLAX_ORIGIN_Y))
            map->parallax_origin.y = tmxParseFloat(value);
        else if (STREQL(name, TMX_WORD_INFINITE))
            map->infinite = tmxParseBool(value);
        else if (STREQL(name, WORD_BACKGROUND_COLOR))
        {
            map->background_color = tmxParseColor(value);
            map->flags |= TMX_FLAG_COLOR;
        }
#ifdef TMX_WARN_UNHANDLED
        else if (!STREQL(name, WORD_NEXT_LAYER_ID) && !STREQL(name, WORD_NEXT_OBJECT_ID) && !STREQL(name, WORD_COMPRESSION_LEVEL))
            tmxXmlWarnAttribute(TMX_WORD_MAP, name);
#endif
    }

    TMXlayer *layer;
    TMXmaptileset mapTileset;

    while (tmxXmlReadElement(context->xml, &name, &size))
    {
        if (STREQL(name, TMX_WORD_PROPERTIES))
        {
            map->properties = tmxXmlParseProperties(context);
            map->flags |= TMX_FLAG_PROPERTIES;
        }
        else if (STREQL(name, TMX_WORD_LAYER) || STREQL(name, WORD_OBJECT_GROUP) || STREQL(name, WORD_IMAGE_LAYER) || STREQL(name, TMX_WORD_GROUP))
        {
            layer = tmxXmlParseLayer(context, name);
            tmxArrayPush(TMXlayer *, map->layers, layer, map->layer_count, layerCapa);
        }
        else if (STREQL(name, TMX_WORD_TILESET))
        {
            mapTileset.tileset = tmxXmlParseTileset(context, &mapTileset.first_gid);
            tmxTilesetConfigureDefaults(mapTileset.tileset, map);
            tmxArrayPush(TMXmaptileset, map->tilesets, mapTileset, map->tileset_count, tilesetCapa);
        }
        else
        {
            tmxXmlWarnElement(TMX_WORD_MAP, name);
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
        if (STREQL(name, TMX_WORD_TILESET))
            template->tileset = tmxXmlParseTileset(context, &template->first_gid);
        else if (STREQL(name, TMX_WORD_OBJECT))
            template->object = tmxXmlParseObject(context);
        else
        {
            tmxXmlWarnElement(TMX_WORD_TEMPLATE, name);
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
    tmxXmlMoveToElement(context->xml, TMX_WORD_MAP);
    map = tmxXmlParseMap(context);
    tmxXmlReaderFree(context->xml);
    return map;
}

TMXtileset *
tmxParseTilesetXml(TMXcontext *context)
{
    TMXtileset *tileset;
    context->xml = tmxXmlReaderInit(context->text);
    tmxXmlMoveToElement(context->xml, TMX_WORD_TILESET);
    tileset = tmxXmlParseTileset(context, NULL);
    tmxXmlReaderFree(context->xml);
    return tileset;
}

TMXtemplate *
tmxParseTemplateXml(TMXcontext *context)
{
    TMXtemplate *template;
    context->xml = tmxXmlReaderInit(context->text);
    tmxXmlMoveToElement(context->xml, TMX_WORD_TEMPLATE);
    template = tmxXmlParseTemplate(context);
    tmxXmlReaderFree(context->xml);
    return template;
}