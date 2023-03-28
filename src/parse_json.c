#define CJSON_HIDE_SYMBOLS 1
#include "cJSON.h"
#include "internal.h"
#include "parse.h"
#include "tmx/compression.h"
#include <ctype.h>

static cJSON_Hooks jsonHooks = {tmxMalloc, tmxFree};

#ifdef TMX_WARN_UNHANDLED
static void
tmxUnhandledProperty(const char *parent, const char *propertyName)
{
    tmxErrorFormat(TMX_ERR_WARN, "Unhandled child property \"%s\" in \"%s\" object.", propertyName, parent);
}
#else
#define tmxUnhandledProperty(parent, propertyName)
#endif

#define JSON_EACH_CHILD(obj, child) for ((child) = (obj)->child; (child); (child) = (child)->next)

static TMX_INLINE char *
JSON_STRING(const cJSON *obj, const char *propName)
{
    if (!obj || !propName)
        return NULL;

    cJSON *child = cJSON_GetObjectItemCaseSensitive(obj, propName);
    if (!child || !child->valuestring)
        return NULL;
    return tmxStringDup(child->valuestring);
}

static TMX_INLINE int
JSON_INTEGER(const cJSON *obj, const char *propName, int ifNone)
{
    if (!obj || !propName)
        return ifNone;

    cJSON *child = cJSON_GetObjectItemCaseSensitive(obj, propName);
    return child ? child->valueint : ifNone;
}

static TMX_INLINE float
JSON_FLOAT(const cJSON *obj, const char *propName)
{
    if (!obj || !propName)
        return 0.0f;

    cJSON *child = cJSON_GetObjectItemCaseSensitive(obj, propName);
    return child ? ((float) child->valuedouble) : 0.0f;
}

#define JSON_BOOL(obj, prop) (JSON_INTEGER((obj), (prop), TMX_FALSE) ? TMX_TRUE : TMX_FALSE)

static TMX_INLINE void
tmxParsePoints(TMXcontext *context, cJSON *polygon, struct TMXcoords *coords)
{
    if (!cJSON_IsArray(polygon))
    {
        coords->count  = 0;
        coords->points = NULL;
        return;
    }

    coords->count  = (size_t) cJSON_GetArraySize(polygon);
    coords->points = tmxMalloc(coords->count * sizeof(TMXvec2));

    int i      = 0;
    float *ptr = (float *) coords->points;

    cJSON *item;
    cJSON_ArrayForEach(item, polygon)
    {
        ptr[i++] = JSON_FLOAT(item, TMX_WORD_X);
        ptr[i++] = JSON_FLOAT(item, TMX_WORD_Y);
    }
}

static TMXproperties *
tmxJsonParseProperties(TMXcontext *context, cJSON *array)
{
    cJSON *item, *value;
    TMXproperties *entry, *properties = NULL;

    if (!array)
        return NULL;
    TMX_ASSERT(cJSON_IsArray(array));

    cJSON_ArrayForEach(item, array)
    {
        entry             = TMX_ALLOC(TMXproperties);
        entry->value.name = JSON_STRING(item, TMX_WORD_NAME);
        entry->key        = entry->value.name;
        if (!entry->value.name)
        {
            tmxError(TMX_ERR_VALUE);
            continue;
        }

        entry->value.class = JSON_STRING(item, WORD_PROPERTY_TYPE);
        entry->value.type  = JSON_INTEGER(item, TMX_WORD_TYPE, TMX_UNSPECIFIED);
        value              = cJSON_GetObjectItemCaseSensitive(item, TMX_WORD_VALUE);

        switch (entry->value.type)
        {
            case TMX_UNSPECIFIED:
            case TMX_PROPERTY_STRING:
            case TMX_PROPERTY_FILE: entry->value.value.string = tmxStringDup(value->valuestring); break;
            case TMX_PROPERTY_INTEGER:
            case TMX_PROPERTY_OBJECT: entry->value.value.integer = value->valueint; break;
            case TMX_PROPERTY_BOOL: entry->value.value.integer = value->valueint ? TMX_TRUE : TMX_FALSE; break;
            case TMX_PROPERTY_FLOAT: entry->value.value.decimal = (float) value->valuedouble; break;
            case TMX_PROPERTY_COLOR: entry->value.value.color = tmxParseColor(value->valuestring); break;
            case TMX_PROPERTY_CLASS: entry->value.value.properties = tmxJsonParseProperties(context, value);
            default:
            {
                tmxFree((void *) entry->key);
                tmxFree(entry);
                tmxError(TMX_ERR_PARAM);
                continue;
            }
        }

        HASH_ADD_KEYPTR(hh, properties, entry->key, strlen(entry->key), entry);
    }
    return properties;
}

static TMXtemplate *
tmxJsonParseTemplate(TMXcontext *context, cJSON *obj)
{
    // TODO
    return NULL;
}

static struct TMXtext *
tmxJsonParseObjectText(TMXcontext *context, cJSON *obj, TMXobject *object)
{
    struct TMXtext *text = TMX_ALLOC(struct TMXtext);
    TMXflag halign       = TMX_ALIGN_LEFT;
    TMXflag valign       = TMX_ALIGN_TOP;
    text->pixel_size     = 16;
    text->kerning        = TMX_TRUE;
    text->wrap           = TMX_TRUE;

    cJSON *child;
    const char *name;

    JSON_EACH_CHILD(obj, child)
    {
        name = child->string;

        if (STREQL(name, TMX_WORD_TEXT))
        {
            object->flags |= TMX_FLAG_TEXT;
            text->string = tmxStringDup(child->valuestring);
        }
        else if (STREQL(name, WORD_PIXEL_SIZE))
        {
            object->flags |= TMX_FLAG_FONT_SIZE;
            text->pixel_size = child->valueint;
        }
        else if (STREQL(name, TMX_WORD_BOLD))
        {
            object->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_BOLD);
            if (child->valueint)
                text->style |= TMX_FONT_STYLE_BOLD;
            else
                text->style &= ~TMX_FONT_STYLE_BOLD;
        }
        else if (STREQL(name, TMX_WORD_ITALIC))
        {
            object->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_ITALIC);
            if (child->valueint)
                text->style |= TMX_FONT_STYLE_ITALIC;
            else
                text->style &= ~TMX_FONT_STYLE_ITALIC;
        }
        else if (STREQL(name, TMX_WORD_UNDERLINE))
        {
            object->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_UNDERLINE);
            if (child->valueint)
                text->style |= TMX_FONT_STYLE_UNDERLINE;
            else
                text->style &= ~TMX_FONT_STYLE_UNDERLINE;
        }
        else if (STREQL(name, TMX_WORD_STRIKEOUT))
        {
            object->flags |= (TMX_FLAG_FONT_STYLE | TMX_FLAG_FONT_STRIKEOUT);
            if (child->valueint)
                text->style |= TMX_FONT_STYLE_STRIKEOUT;
            else
                text->style &= ~TMX_FONT_STYLE_STRIKEOUT;
        }
        else if (STREQL(name, WORD_FONT_FAMILY))
        {
            object->flags |= TMX_FLAG_FONT;
            text->font = tmxStringDup(child->valuestring);
        }
        else if (STREQL(name, TMX_WORD_HALIGN))
        {
            object->flags |= (TMX_FLAG_ALIGN | TMX_FLAG_HALIGN);
            halign = tmxParseAlignH(child->valuestring);
        }
        else if (STREQL(name, TMX_WORD_VALIGN))
        {
            object->flags |= (TMX_FLAG_ALIGN | TMX_FLAG_VALIGN);
            valign = tmxParseAlignV(child->valuestring);
        }
        else if (STREQL(name, TMX_WORD_KERNING))
        {
            object->flags |= TMX_FLAG_FONT_KERNING;
            text->kerning = child->valueint ? TMX_TRUE : TMX_FALSE;
        }
        else if (STREQL(name, TMX_WORD_WRAP))
        {
            object->flags |= TMX_FLAG_WORD_WRAP;
            text->wrap = child->valueint ? TMX_TRUE : TMX_FALSE;
        }
        else if (STREQL(name, TMX_WORD_COLOR))
        {
            object->flags |= TMX_FLAG_COLOR;
            text->color = tmxParseColor(child->valuestring);
        }
        else
        {
            tmxUnhandledProperty(TMX_WORD_TEXT, name);
        }
    }

    text->align = (halign | valign);
    return text;
}

static TMXobject *
tmxJsonParseObject(TMXcontext *context, cJSON *obj)
{
    TMXobject *object = TMX_ALLOC(TMXobject);

    cJSON *child;
    const char *name;

    JSON_EACH_CHILD(obj, child)
    {
        name = child->string;

        if (STREQL(name, TMX_WORD_ID))
        {
            object->id = child->valueint;
        }
        else if (STREQL(name, TMX_WORD_NAME))
        {
            object->flags |= TMX_FLAG_NAME;
            object->name = tmxStringDup(child->valuestring);
        }
        else if (STREQL(name, TMX_WORD_X))
        {
            object->flags |= (TMX_FLAG_X | TMX_FLAG_POSITION);
            object->position.x = (float) child->valuedouble;
        }
        else if (STREQL(name, TMX_WORD_Y))
        {
            object->flags |= (TMX_FLAG_Y | TMX_FLAG_POSITION);
            object->position.y = (float) child->valuedouble;
        }
        else if (STREQL(name, TMX_WORD_WIDTH))
        {
            object->flags |= (TMX_FLAG_WIDTH | TMX_FLAG_SIZE);
            object->size.x = (float) child->valuedouble;
        }
        else if (STREQL(name, TMX_WORD_HEIGHT))
        {
            object->flags |= (TMX_FLAG_HEIGHT | TMX_FLAG_SIZE);
            object->size.y = (float) child->valuedouble;
        }
        else if (STREQL(name, TMX_WORD_TYPE) || STREQL(name, TMX_WORD_CLASS))
        {
            object->flags |= TMX_FLAG_CLASS;
            object->class = tmxStringDup(child->valuestring);
        }
        else if (STREQL(name, TMX_WORD_VISIBLE))
        {
            object->flags |= TMX_FLAG_VISIBLE;
            object->visible = child->valueint ? TMX_TRUE : TMX_FALSE;
        }
        else if (STREQL(name, TMX_WORD_GID))
        {
            object->flags |= TMX_FLAG_GID;
            object->gid = (TMXgid) child->valueint;
        }
        else if (STREQL(name, TMX_WORD_ROTATION))
        {
            object->flags |= TMX_FLAG_ROTATION;
            object->rotation = (float) child->valuedouble;
        }
        else if (STREQL(name, TMX_WORD_PROPERTIES))
        {
            object->flags |= TMX_FLAG_PROPERTIES;
            object->properties = tmxJsonParseProperties(context, child);
        }
        else if (STREQL(name, TMX_WORD_POINT) && child->valueint)
        {
            TMX_ASSERT(object->type == TMX_UNSPECIFIED);
            object->type = TMX_OBJECT_POINT;
        }
        else if (STREQL(name, TMX_WORD_ELLIPSE) && child->valueint)
        {
            TMX_ASSERT(object->type == TMX_UNSPECIFIED);
            object->type = TMX_OBJECT_ELLIPSE;
        }
        else if (STREQL(name, TMX_WORD_POLYGON))
        {
            TMX_ASSERT(object->type == TMX_UNSPECIFIED);
            object->type = TMX_OBJECT_POLYGON;
            tmxParsePoints(context, child, &object->poly);
        }
        else if (STREQL(name, TMX_WORD_POLYLINE))
        {
            TMX_ASSERT(object->type == TMX_UNSPECIFIED);
            object->type = TMX_OBJECT_POLYLINE;
            tmxParsePoints(context, child, &object->poly);
        }
        else if (STREQL(name, TMX_WORD_TEMPLATE))
        {
            char templatePath[TMX_MAX_PATH];
            tmxFileAbsolutePath(child->valuestring, context->basePath, templatePath, TMX_MAX_PATH);
            object->template = tmxLoadTemplate(templatePath, context->cache, TMX_FORMAT_AUTO);
        }
        else if (STREQL(name, TMX_WORD_TEXT))
        {
            TMX_ASSERT(object->type == TMX_UNSPECIFIED);
            object->type = TMX_OBJECT_TEXT;
            object->text = tmxJsonParseObjectText(context, child, object);
        }
        else
        {
            tmxUnhandledProperty(TMX_WORD_OBJECT, name);
        }
    }

    if (object->template && object->template->object)
        tmxObjectMergeTemplate(object, object->template->object);

    return object;
}

static TMX_INLINE void
tmxJsonParseDataType(cJSON *obj, TMXenum *encoding, TMXenum *compression)
{
    *encoding    = TMX_ENCODING_NONE;
    *compression = TMX_COMPRESSION_NONE;

    cJSON *child;
    child = cJSON_GetObjectItemCaseSensitive(obj, TMX_WORD_ENCODING);
    if (child)
        *encoding = tmxParseEncoding(child->valuestring);

    child = cJSON_GetObjectItemCaseSensitive(obj, TMX_WORD_COMPRESSION);
    if (child)
        *compression = tmxParseCompression(child->valuestring);
}

static TMXgid *
tmxJsonParseTileData(cJSON *obj, TMXenum encoding, TMXenum compression, size_t count)
{
    if (!count)
        return NULL;

    cJSON *child;
    size_t i     = 0;
    TMXgid *gids = tmxMalloc(count * sizeof(TMXgid));

    if (cJSON_IsArray(obj))
    {
        TMX_ASSERT(count == cJSON_GetArraySize(obj));
        cJSON_ArrayForEach(child, obj) gids[i++] = (TMXgid) child->valueint;
        return gids;
    }

    TMX_ASSERT(cJSON_IsString(obj));
    char *str = obj->valuestring;

    // Ignore leading whitespace
    while (isspace(*str))
    {
        str++;
    }

    // Ignore trailing whitespace
    size_t len = strlen(str);
    while (len > 0 && isspace(str[len - 1]))
    {
        len--;
    }

    TMX_ASSERT(tmxInflate(str, len, gids, count, compression) == count);
    return gids;
}

static TMXlayer *
tmxJsonParseLayer(TMXcontext *context, cJSON *obj)
{
    TMXlayer *layer = TMX_ALLOC(TMXlayer);
    layer->parallax = (TMXvec2){1.0f, 1.0f};
    layer->visible  = TMX_TRUE;
    layer->opacity  = 1.0f;

    size_t i;
    cJSON *child, *arrayChild;
    const char *name;
    TMXenum encoding, compression;

    child = cJSON_GetObjectItemCaseSensitive(obj, TMX_WORD_TYPE);
    if (child)
    {
        TMXbool infinite = TMX_FALSE;
        if (context->map)
            infinite = context->map->infinite;
        else
            infinite = cJSON_GetObjectItemCaseSensitive(obj, TMX_WORD_CHUNKS) ? TMX_TRUE : TMX_FALSE;

        layer->type = tmxParseLayerType(child->valuestring, infinite);
    }

    // Unlike in the XML, images are not independent objects in the JSON spec
    if (layer->type == TMX_LAYER_IMAGE)
        layer->data.image = TMX_ALLOC(TMXimage);

    JSON_EACH_CHILD(obj, child)
    {
        name = child->string;
        if (STREQL(name, TMX_WORD_ID))
            layer->id = child->valueint;
        else if (STREQL(name, TMX_WORD_NAME))
            layer->name = tmxStringDup(child->valuestring);
        else if (STREQL(name, TMX_WORD_CLASS))
            layer->class = tmxStringDup(child->valuestring);
        else if (STREQL(name, WORD_DRAW_ORDER))
            layer->draw_order = tmxParseDrawOrder(child->valuestring);
        else if (STREQL(name, TMX_WORD_IMAGE))
        {
            TMX_ASSERT(layer->type == TMX_LAYER_IMAGE);
            layer->data.image->flags |= TMX_FLAG_EXTERNAL;
            layer->data.image->source = tmxStringDup(child->valuestring);
        }
        else if (STREQL(name, WORD_TRANSPARENT_COLOR))
        {
            TMX_ASSERT(layer->type == TMX_LAYER_IMAGE);
            layer->data.image->flags |= TMX_FLAG_COLOR;
            layer->data.image->transparent = tmxParseColor(child->valuestring);
        }
        else if (STREQL(name, WORD_TINT_COLOR))
        {
            layer->flags |= TMX_FLAG_COLOR;
            layer->tint_color = tmxParseColor(child->valuestring);
        }
        else if (STREQL(name, WORD_PARALLAX_X))
            layer->parallax.x = (float) child->valuedouble;
        else if (STREQL(name, WORD_PARALLAX_Y))
            layer->parallax.y = (float) child->valuedouble;
        else if (STREQL(name, TMX_WORD_VISIBLE))
            layer->visible = child->valueint ? TMX_TRUE : TMX_FALSE;
        else if (STREQL(name, WORD_OFFSET_X))
            layer->offset.x = child->valueint;
        else if (STREQL(name, WORD_OFFSET_Y))
            layer->offset.y = child->valueint;
        else if (STREQL(name, TMX_WORD_X))
            layer->position.x = child->valueint;
        else if (STREQL(name, TMX_WORD_Y))
            layer->position.y = child->valueint;
        else if (STREQL(name, TMX_WORD_WIDTH))
            layer->size.w = child->valueint;
        else if (STREQL(name, TMX_WORD_HEIGHT))
            layer->size.h = child->valueint;
        else if (STREQL(name, TMX_WORD_OPACITY))
            layer->opacity = (float) TMX_CLAMP(child->valuedouble, 0.0, 1.0);
        else if (STREQL(name, WORD_REPEAT_X))
            layer->repeat.x = child->valueint ? TMX_TRUE : TMX_FALSE;
        else if (STREQL(name, WORD_REPEAT_Y))
            layer->repeat.y = child->valueint ? TMX_TRUE : TMX_FALSE;
        else if (STREQL(name, TMX_WORD_PROPERTIES))
            layer->properties = tmxJsonParseProperties(context, child);
        else if (STREQL(name, TMX_WORD_OBJECTS))
        {
            TMX_ASSERT(layer->type == TMX_LAYER_OBJGROUP);
            TMX_ASSERT(cJSON_IsArray(child));

            layer->count = (size_t) cJSON_GetArraySize(child);
            if (!layer->count)
                continue;
            layer->data.objects = tmxMalloc(layer->count * sizeof(TMXobject *));

            i = 0;
            cJSON_ArrayForEach(arrayChild, child) { layer->data.objects[i++] = tmxJsonParseObject(context, arrayChild); }
        }
        else if (STREQL(name, TMX_WORD_LAYERS))
        {
            TMX_ASSERT(layer->type == TMX_LAYER_GROUP);
            TMX_ASSERT(cJSON_IsArray(child));

            layer->count = (size_t) cJSON_GetArraySize(child);
            if (!layer->count)
                continue;
            layer->data.group = tmxMalloc(layer->count * sizeof(TMXlayer *));

            i = 0;
            cJSON_ArrayForEach(arrayChild, child) { layer->data.group[i++] = tmxJsonParseLayer(context, arrayChild); }
        }
        else if (STREQL(name, TMX_WORD_DATA))
        {
            tmxJsonParseDataType(obj, &encoding, &compression);

            if (!layer->size.w)
                layer->size.w = JSON_INTEGER(obj, TMX_WORD_WIDTH, 0);
            if (!layer->size.h)
                layer->size.h = JSON_INTEGER(obj, TMX_WORD_HEIGHT, 0);

            layer->count      = layer->size.w * layer->size.h;
            layer->data.tiles = tmxJsonParseTileData(child, encoding, compression, layer->count);
        }
        else if (STREQL(name, TMX_WORD_CHUNKS))
        {
            tmxJsonParseDataType(obj, &encoding, &compression);

            TMX_ASSERT(cJSON_IsArray(child));
            layer->count = (size_t) cJSON_GetArraySize(child);
            if (!layer->count)
                continue;

            i = 0;
            TMXchunk *chunk;
            cJSON *data;
            layer->data.chunks = tmxMalloc(layer->count * sizeof(TMXchunk));
            cJSON_ArrayForEach(arrayChild, child)
            {
                chunk           = &layer->data.chunks[i++];
                chunk->bounds.x = JSON_INTEGER(arrayChild, TMX_WORD_X, 0);
                chunk->bounds.y = JSON_INTEGER(arrayChild, TMX_WORD_Y, 0);
                chunk->bounds.w = JSON_INTEGER(arrayChild, TMX_WORD_WIDTH, 0);
                chunk->bounds.h = JSON_INTEGER(arrayChild, TMX_WORD_HEIGHT, 0);
                chunk->count    = chunk->bounds.w * chunk->bounds.h;

                if (!chunk->count)
                {
                    chunk->gids = NULL;
                    continue;
                }

                data        = cJSON_GetObjectItemCaseSensitive(arrayChild, TMX_WORD_DATA);
                chunk->gids = tmxJsonParseTileData(data, encoding, compression, chunk->count);
            }
        }
        // These are handled elsewhere
        else if (STREQL(name, TMX_WORD_ENCODING) || STREQL(name, TMX_WORD_COMPRESSION) || STREQL(name, TMX_WORD_TYPE))
            continue;
        // These are irrelevant to the API
        else if (STREQL(name, "startx") || STREQL(name, "starty") || STREQL(name, "locked"))
            continue;
        else
        {
            tmxUnhandledProperty(TMX_WORD_LAYER, name);
            continue;
        }
    }

    if (layer->type == TMX_LAYER_IMAGE)
        tmxImageUserLoad(layer->data.image, context->basePath);

    return layer;
}

static void
tmxJsonParseCollision(TMXcontext *context, cJSON *obj, TMXcollision *collision)
{
    TMX_ASSERT(cJSON_IsObject(obj));
    cJSON *child, *arrayChild;
    const char *name;

    size_t capacity    = 4;
    collision->objects = tmxMalloc(capacity * sizeof(TMXobject *));

    JSON_EACH_CHILD(obj, child)
    {
        name = child->string;
        if (STREQL(name, TMX_WORD_ID) || STREQL(name, TMX_WORD_NAME) || STREQL(name, WORD_DRAW_ORDER))
            continue;

        if (!STREQL(name, TMX_WORD_OBJECTS))
        {
            tmxUnhandledProperty(WORD_OBJECT_GROUP, name);
            continue;
        }

        TMXobject *object;
        TMX_ASSERT(cJSON_IsArray(child));
        cJSON_ArrayForEach(arrayChild, child)
        {
            object = tmxJsonParseObject(context, arrayChild);
            if (!object)
                continue;
            tmxArrayPush(TMXobject *, collision->objects, object, collision->count, capacity);
        }
    }

    tmxArrayFinish(TMXobject *, collision->objects, collision->count, capacity);
    if (!collision->count)
        collision->objects = NULL;
}

static void
tmxJsonParseTile(TMXcontext *context, cJSON *obj, TMXtile *tiles, TMXbool isCollection, size_t tileIndex)
{
    cJSON *child;
    const char *name;
    TMXtile *tile;
    TMXimage *image;

    TMXtid id = (TMXtid) JSON_INTEGER(obj, TMX_WORD_ID, 0);
    if (!isCollection)
        tileIndex = id;

    tile     = &tiles[tileIndex];
    tile->id = id;

    JSON_EACH_CHILD(obj, child)
    {
        name = child->string;
        if (STREQL(name, TMX_WORD_ID))
            continue;
        else if (STREQL(name, TMX_WORD_X))
            tile->rect.x = child->valueint;
        else if (STREQL(name, TMX_WORD_Y))
            tile->rect.y = child->valueint;
        else if (STREQL(name, TMX_WORD_WIDTH))
            tile->rect.w = child->valueint;
        else if (STREQL(name, TMX_WORD_HEIGHT))
            tile->rect.h = child->valueint;
        else if (STREQL(name, TMX_WORD_TYPE))
            tile->class = tmxStringDup(child->valuestring);
        else if (STREQL(name, TMX_WORD_IMAGE))
        {
            if (!image)
                image = TMX_ALLOC(TMXimage);
            image->source = tmxStringDup(child->valuestring);
        }
        else if (STREQL(name, WORD_IMAGE_WIDTH))
        {
            if (!image)
                image = TMX_ALLOC(TMXimage);
            image->size.w = child->valueint;
        }
        else if (STREQL(name, WORD_IMAGE_HEIGHT))
        {
            if (!image)
                image = TMX_ALLOC(TMXimage);
            image->size.h = child->valueint;
        }
        else if (STREQL(name, TMX_WORD_PROPERTIES))
        {
            tile->properties = tmxJsonParseProperties(context, child);
        }
        else if (STREQL(name, TMX_WORD_ANIMATION))
        {
            TMX_ASSERT(cJSON_IsArray(child));
            tile->animation.count = (size_t) cJSON_GetArraySize(child);
            if (!tile->animation.count)
                continue;

            int i = 0;
            cJSON *frameChild;
            tile->animation.frames = tmxMalloc(tile->animation.count * sizeof(TMXvec2));
            cJSON_ArrayForEach(frameChild, child)
            {
                tile->animation.frames[i].duration = JSON_INTEGER(frameChild, TMX_WORD_DURATION, 0);
                tile->animation.frames[i].id       = (TMXtid) JSON_INTEGER(frameChild, TMX_WORD_ID, 0);
                i++;
            }
        }
        else if (STREQL(name, WORD_OBJECT_GROUP))
        {
            tmxJsonParseCollision(context, child, &tile->collision);
        }
        else if (!STREQL(name, TMX_WORD_PROBABILITY) && !STREQL(name, TMX_WORD_TERRAIN))
        {
            tmxUnhandledProperty(TMX_WORD_TILE, name);
        }
    }

    // TODO: Set default size if image defined and not size defined
}

static TMXtileset *
tmxJsonParseTileset(TMXcontext *context, cJSON *obj, TMXgid *firstGid)
{
    TMXtileset *tileset;
    TMXimage *image = NULL;

    cJSON *child, *tiles = NULL;
    const char *name;

    // Ensure first GID is read and set before "source" property is encountered.
    if (firstGid)
    {
        child = cJSON_GetObjectItemCaseSensitive(obj, WORD_FIRST_GID);
        if (child)
            *firstGid = (TMXgid) child->valueint;
    }

    // Check for a "source" property, if so, build absolute path and load it.
    child = cJSON_GetObjectItemCaseSensitive(obj, TMX_WORD_SOURCE);
    if (child)
    {
        // External tileset
        char tilesetPath[TMX_MAX_PATH];
        tmxFileAbsolutePath(child->valuestring, context->basePath, tilesetPath, TMX_MAX_PATH);
        tileset = tmxLoadTileset(tilesetPath, context->cache, TMX_FORMAT_AUTO);
        if (tileset)
            tileset->flags |= TMX_FLAG_EXTERNAL;
        return tileset;
    }

    tileset = TMX_ALLOC(TMXtileset);
    JSON_EACH_CHILD(obj, child)
    {
        name = child->string;
        if (STREQL(name, TMX_WORD_NAME))
            tileset->name = tmxStringDup(child->valuestring);
        else if (STREQL(name, TMX_WORD_CLASS))
            tileset->class = tmxStringDup(child->valuestring);
        else if (STREQL(name, TMX_WORD_PROPERTIES))
        {
            tileset->properties = tmxJsonParseProperties(context, child);
            tileset->flags |= TMX_FLAG_PROPERTIES;
        }
        else if (STREQL(name, WORD_BACKGROUND_COLOR))
        {
            tileset->background_color = tmxParseColor(child->valuestring);
            tileset->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, TMX_WORD_COLUMNS))
            tileset->columns = child->valueint;
        else if (STREQL(name, TMX_WORD_MARGIN))
            tileset->margin = child->valueint;
        else if (STREQL(name, TMX_WORD_SPACING))
            tileset->spacing = child->valueint;
        else if (STREQL(name, TMX_WORD_TYPE))
        {
            TMX_ASSERT(STREQL(TMX_WORD_TILESET, child->valuestring));
        }
        else if (STREQL(name, TMX_WORD_VERSION))
            tileset->version = tmxStringDup(child->valuestring);
        else if (STREQL(name, WORD_TILED_VERSION))
            tileset->tiled_version = tmxStringDup(child->valuestring);
        else if (STREQL(name, WORD_TILE_RENDER_SIZE))
            tileset->render_size = tmxParseRenderSize(child->valuestring);
        else if (STREQL(name, TMX_WORD_IMAGE))
        {
            if (!image)
                image = TMX_ALLOC(TMXimage);
            image->source = tmxStringDup(child->valuestring);
        }
        else if (STREQL(name, WORD_IMAGE_WIDTH))
        {
            if (!image)
                image = TMX_ALLOC(TMXimage);
            image->size.w = child->valueint;
        }
        else if (STREQL(name, WORD_IMAGE_HEIGHT))
        {
            if (!image)
                image = TMX_ALLOC(TMXimage);
            image->size.h = child->valueint;
        }
        else if (STREQL(name, WORD_TRANSPARENT_COLOR))
        {
            if (!image)
                image = TMX_ALLOC(TMXimage);
            image->flags |= TMX_FLAG_COLOR;
            image->transparent = tmxParseColor(child->valuestring);
        }
        else if (STREQL(name, WORD_FILL_MODE))
            tileset->fill_mode = tmxParseFillMode(child->valuestring);
        else if (STREQL(name, WORD_OBJECT_ALIGN))
            tileset->object_align = tmxParseObjectAlignment(child->valuestring);
        else if (STREQL(name, WORD_TILE_COUNT))
            tileset->tile_count = (size_t) child->valueint;
        else if (STREQL(name, WORD_TILE_WIDTH))
            tileset->tile_size.w = child->valueint;
        else if (STREQL(name, WORD_TILE_HEIGHT))
            tileset->tile_size.h = child->valueint;
        else if (STREQL(name, TMX_WORD_GRID))
        {
            tileset->grid.orientation = tmxParseOrientation(JSON_STRING(child, TMX_WORD_ORIENTATION));
            tileset->grid.size.w      = JSON_INTEGER(child, TMX_WORD_WIDTH, 0);
            tileset->grid.size.h      = JSON_INTEGER(child, TMX_WORD_HEIGHT, 0);
        }
        else if (STREQL(name, WORD_TILE_OFFSET))
        {
            tileset->offset.x = JSON_INTEGER(child, TMX_WORD_X, 0);
            tileset->offset.y = JSON_INTEGER(child, TMX_WORD_Y, 0);
        }
        else if (STREQL(name, TMX_WORD_TILES))
        {
            // Store in a variable, but do nothing. Post-process tiles after all other properties have been defined.
            tiles = child;
        }
        else if (!STREQL(name, TMX_WORD_TRANSFORMATIONS) && !STREQL(name, TMX_WORD_WANGSETS) && !STREQL(name, TMX_WORD_TERRAINS))
        {
            tmxUnhandledProperty(TMX_WORD_TILESET, name);
        }
    }

    if (image)
    {
        tmxImageUserLoad(image, context->basePath);
        tileset->image = image;
    }

    if (tiles)
    {
        TMX_ASSERT(cJSON_IsArray(tiles));

        size_t tileIndex     = 0;
        TMXbool isCollection = (TMXbool) tileset->columns == 0;
        tmxInitTilesetTiles(tileset, isCollection);
        cJSON_ArrayForEach(child, tiles) { tmxJsonParseTile(context, child, tileset->tiles, isCollection, tileIndex++); }
    }

    return tileset;
}

/*

tiles
array
Array of Tiles (optional)

 */

static TMXmap *
tmxJsonParseMap(TMXcontext *context, cJSON *obj)
{
    TMXmap *map  = TMX_ALLOC(TMXmap);
    context->map = map;

    size_t i;
    cJSON *child, *arrayItem;
    const char *name;
    JSON_EACH_CHILD(obj, child)
    {
        name = child->string;

        if (STREQL(name, TMX_WORD_VERSION))
            map->version = tmxStringDup(child->valuestring);
        else if (STREQL(name, WORD_TILED_VERSION))
            map->tiled_version = tmxStringDup(child->valuestring);
        else if (STREQL(name, TMX_WORD_CLASS))
            map->class = tmxStringDup(child->valuestring);
        else if (STREQL(name, TMX_WORD_WIDTH))
            map->size.w = child->valueint;
        else if (STREQL(name, TMX_WORD_HEIGHT))
            map->size.h = child->valueint;
        else if (STREQL(name, WORD_BACKGROUND_COLOR))
        {
            map->background_color = tmxParseColor(child->valuestring);
            map->flags |= TMX_FLAG_COLOR;
        }
        else if (STREQL(name, WORD_TILE_WIDTH))
            map->tile_size.w = child->valueint;
        else if (STREQL(name, WORD_TILE_HEIGHT))
            map->tile_size.h = child->valueint;
        else if (STREQL(name, TMX_WORD_INFINITE))
            map->infinite = child->valueint ? TMX_TRUE : TMX_FALSE;
        else if (STREQL(name, TMX_WORD_ORIENTATION))
            map->orientation = tmxParseOrientation(child->valuestring);
        else if (STREQL(name, WORD_PARALLAX_ORIGIN_X))
            map->parallax_origin.x = (float) cJSON_GetNumberValue(child);
        else if (STREQL(name, WORD_PARALLAX_ORIGIN_Y))
            map->parallax_origin.y = (float) cJSON_GetNumberValue(child);
        else if (STREQL(name, WORD_RENDER_ORDER))
            map->orientation = tmxParseRenderOrder(child->valuestring);
        else if (STREQL(name, WORD_STAGGER_AXIS))
            map->stagger.axis = tmxParseStaggerAxis(child->valuestring);
        else if (STREQL(name, WORD_STAGGER_INDEX))
            map->stagger.index = tmxParseStaggerIndex(child->valuestring);
        else if (STREQL(name, WORD_HEX_SIDE_LENGTH))
            map->hex_side = child->valueint;
        else if (STREQL(name, TMX_WORD_PROPERTIES))
            map->properties = tmxJsonParseProperties(context, child);
        else if (STREQL(name, WORD_NEXT_LAYER_ID) || STREQL(name, WORD_NEXT_OBJECT_ID) || STREQL(name, WORD_COMPRESSION_LEVEL))
            continue;
        else if (STREQL(name, TMX_WORD_TYPE))
        {
            TMX_ASSERT(STREQL(child->valuestring, TMX_WORD_MAP));
            continue;
        }
        else if (STREQL(name, TMX_WORD_LAYERS))
        {
            TMX_ASSERT(cJSON_IsArray(child));
            map->layer_count = (size_t) cJSON_GetArraySize(child);
            if (!map->layer_count)
                continue;

            i           = 0;
            map->layers = tmxMalloc(map->layer_count * sizeof(TMXlayer *));
            cJSON_ArrayForEach(arrayItem, child) { map->layers[i++] = tmxJsonParseLayer(context, arrayItem); }
        }
        else if (STREQL(name, TMX_WORD_TILESETS))
        {
            TMX_ASSERT(cJSON_IsArray(child));
            map->tileset_count = (size_t) cJSON_GetArraySize(child);
            if (!map->tileset_count)
                continue;

            i = 0;
            TMXmaptileset *mapTileset;
            map->tilesets = tmxMalloc(map->tileset_count * sizeof(TMXmaptileset));
            cJSON_ArrayForEach(arrayItem, child)
            {
                mapTileset          = &map->tilesets[i++];
                mapTileset->tileset = tmxJsonParseTileset(context, arrayItem, &mapTileset->first_gid);
            }
        }
        else
        {
            tmxUnhandledProperty(TMX_WORD_MAP, name);
        }
    }

    map->pixel_size = (TMXsize){map->size.w * map->tile_size.w, map->size.h * map->tile_size.h};
    return map;
}

TMXmap *
tmxParseMapJson(TMXcontext *context)
{
    TMXmap *map;
    cJSON_InitHooks(&jsonHooks);
    context->json = cJSON_Parse(context->text);
    TMX_ASSERT(context->json);
    map = tmxJsonParseMap(context, context->json);
    cJSON_Delete(context->json);
    return map;
}

TMXtileset *
tmxParseTilesetJson(TMXcontext *context)
{
    TMXtileset *tileset;
    cJSON_InitHooks(&jsonHooks);
    context->json = cJSON_Parse(context->text);
    TMX_ASSERT(context->json);
    tileset = tmxJsonParseTileset(context, context->json, NULL);
    cJSON_Delete(context->json);
    return tileset;
}

TMXtemplate *
tmxParseTemplateJson(TMXcontext *context)
{
    TMXtemplate *template;
    cJSON_InitHooks(&jsonHooks);
    context->json = cJSON_Parse(context->text);
    TMX_ASSERT(context->json);
    template = tmxJsonParseTemplate(context, context->json);
    cJSON_Delete(context->json);
    return template;
}