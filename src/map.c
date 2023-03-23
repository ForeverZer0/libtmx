#include "internal.h"
#include "tmx.h"
#include "tmx/compression.h"
#include "tmx/error.h"
#include "tmx/memory.h"
#include "tmx/properties.h"
#include "tmx/xml.h"
#include <stdio.h>
#include <string.h>

TMXproperties *tmxXmlReadProperties(TMXxmlreader *xml);

static inline TMXbool
tmxAssertElement(const char *actual, const char *expected)
{
    if (STREQL(actual, expected))
        return TMX_TRUE;

    tmxErrorFormat(TMX_ERR_FORMAT, "Expected <%s> element, encountered <%s>.", expected, actual);
    return TMX_FALSE;
}

void
tmxMapFree(TMXmap *map)
{
    if (!map)
        return;

    tmxFree(map);
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

static TMXflag
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

static TMXflag
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

static struct TMXtext *
tmxXmlReadText(TMXxmlreader *xml)
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

    while (tmxXmlReadAttr(xml, &name, &value))
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
    }
    text->align = (halign | valign);

    if (tmxXmlMoveToContent(xml))
    {
        if (tmxXmlReadStringContents(xml, &value, &size, TMX_FALSE))
            text->string = tmxStringCopy(value, size);
    }

    return text;
}

static void
tmxObjectFromTemplate(TMXobject *dst, TMXobject *src)
{
}

/*
id, x, and y are always redefined
*/

TMXobject *
tmxXmlReadObject(TMXxmlreader *xml, TMXcontext *context)
{
    TMXobject *object = TMX_CALLOC(TMXobject);

    const char *name;
    const char *value;
    size_t size;

    while (tmxXmlReadAttr(xml, &name, &value))
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
            object->gid = (TMXgid) strtoul(value, NULL, 10);
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
    }

    // Move to contents. Return early if there is none.
    if (!tmxXmlMoveToContent(xml))
        return object;

    while (tmxXmlReadElement(xml, &name, &size))
    {
        if (STREQL(name, "properties"))
            object->properties = tmxXmlReadProperties(xml);
        else if (STREQL(name, "point"))
            object->type = TMX_OBJECT_POINT;
        else if (STREQL(name, "ellipse"))
            object->type = TMX_OBJECT_ELLIPSE;
        else if (STREQL(name, "polygon") || STREQL(name, "polyline"))
        {
            object->type = STREQL(name, "polygon") ? TMX_OBJECT_POLYGON : TMX_OBJECT_POLYLINE;
            tmxXmlReadAttr(xml, &name, &value);
            tmxXmlMoveToContent(xml);
            // It is safe to stomp all over this pointer, it is temporary and no longer valid on the next loop
            tmxParsePoints((char *) value, &object->points);
            object->flags |= TMX_FLAG_POINTS;
        }
        else if (STREQL(name, "text"))
        {
            object->type = TMX_OBJECT_TEXT;
            object->flags |= TMX_FLAG_TEXT;
            object->text = tmxXmlReadText(xml);
        }
    }

    return object;
}

TMXlayer *
tmxXmlReadLayer(TMXxmlreader *xml, TMXcontext *context, const char *layerType)
{
    const char *name;
    const char *value;
    size_t size;

    TMXlayer *layer = TMX_CALLOC(TMXlayer);
    layer->type     = tmxParseLayerType(layerType, context->map->infinite);

    while (tmxXmlReadAttr(xml, &name, &value))
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
    }

    if (!tmxXmlMoveToContent(xml))
    {
        tmxError(TMX_ERR_PARSE);
        return layer;
    }

    TMXobject *previousObject = NULL;

    while (tmxXmlReadElement(xml, &name, &size))
    {
        if (STREQL(name, "properties"))
            layer->properties = tmxXmlReadProperties(xml);
        else if (STREQL(name, "data"))
        {
            // <layer>
            // tile/chunk
        }
        else if (STREQL(name, "object"))
        {
            TMXobject *obj = tmxXmlReadObject(xml, context);
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
        else if (STREQL(name, "image"))
        {
            // <imagelayer>
        }
        else if (STREQL(name, "layer") || STREQL(name, "objectgroup") || STREQL(name, "imagelayer") || STREQL(name, "group"))
        {
            // <group>
        }
        else
        {
            tmxError(TMX_ERR_PARSE);
        }
    }

    return layer;
}

TMXmap *
tmxMapLoad(const char *filename)
{
    TMXxmlreader *xml = tmxXmlReaderInitFromFile(filename, 32768);
    TMXmap *map       = TMX_CALLOC(TMXmap);

    TMXcontext context;
    context.map     = map;
    context.cache   = NULL;
    context.baseDir = "";
    // TODO

    const char *name;
    const char *value;
    size_t size;

    if (tmxXmlReadElement(xml, &name, &size))
    {
        if (!tmxAssertElement(name, "map"))
            return NULL;

        while (tmxXmlReadAttr(xml, &name, &value))
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
        }

        if (!tmxXmlMoveToContent(xml))
            return map;

        TMXlayer *layer;
        TMXlayer *previous = NULL;

        while (tmxXmlReadElement(xml, &name, &size))
        {
            if (STREQL(name, "properties"))
                map->properties = tmxXmlReadProperties(xml);
            else if (STREQL(name, "layer") || STREQL(name, "objectgroup") || STREQL(name, "imagelayer") || STREQL(name, "group"))
            {
                layer = tmxXmlReadLayer(xml, &context, name);
                if (!layer)
                {
                    tmxErrorFormat(TMX_ERR_PARSE, "Failed to parse %s layer", name);
                    continue;
                }
                if (previous)
                    previous->next = layer;
                else
                    map->layers = layer;
                previous = layer;
                map->count++;
            }
        }

        tmxXmlMoveToContent(xml);
        tmxXmlAssertEOF(xml);
    }

    return map;
}
