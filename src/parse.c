#include "parse.h"
#include "cwalk.h"
#include "internal.h"
#include <ctype.h>

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
	uint32_t u32 = (uint32_t) tmxParseUint(str);
	if (len < 6) {
		u32 = (u32 & 0xF000u) << 16 | (u32 & 0xF000u) << 12
		    | (u32 & 0x0F00u) << 12 | (u32 & 0x0F00u) <<  8
		    | (u32 & 0x00F0u) <<  8 | (u32 & 0x00F0u) <<  4
		    | (u32 & 0x000Fu) <<  4 | (u32 & 0x000Fu);     
	}
	if (len == 6 || len == 3)
        u32 |= 0xFF000000u;

    // If using packed colors, just assign the value, else normalize it to 0.0 - 1.0.
    #ifdef TMX_VECTOR_COLOR
    color.a = ((u32 >> 24) & 0xFF) / 255.0f;
    color.r = ((u32 >> 16) & 0xFF) / 255.0f;
    color.g = ((u32 >>  8) & 0xFF) / 255.0f;
    color.b = ((u32 >>  0) & 0xFF) / 255.0f;
    #else
    color.value = u32;
    #endif

    // clang-format on

    return color;
}

static void
tmxErrorInvalidEnum(const char *enumName, const char *value)
{
    tmxErrorFormat(TMX_ERR_VALUE, "Unrecognized \"%s\" \"%s\" specified.", value ? value : ""); // TODO
}

TMX_PROPERTY_TYPE
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

    tmxErrorInvalidEnum("property type", value);
    return TMX_PROPERTY_UNSPECIFIED;
}

TMX_ORIENTATION
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

    tmxErrorInvalidEnum("orientation", value);
    return TMX_ORIENTATION_UNSPECIFIED;
}

TMX_RENDER_ORDER
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

    tmxErrorInvalidEnum("render order", value);
    return TMX_UNSPECIFIED;
}

TMX_STAGGER_AXIS
tmxParseStaggerAxis(const char *value)
{
    if (STREQL(value, "x"))
        return TMX_STAGGER_AXIS_X;
    if (STREQL(value, "y"))
        return TMX_STAGGER_AXIS_Y;

    tmxErrorInvalidEnum("stagger axis", value);
    return TMX_UNSPECIFIED;
}

TMX_STAGGER_INDEX
tmxParseStaggerIndex(const char *value)
{
    if (STREQL(value, "even"))
        return TMX_STAGGER_INDEX_EVEN;
    if (STREQL(value, "odd"))
        return TMX_STAGGER_INDEX_ODD;

    tmxErrorInvalidEnum("stagger index", value);
    return TMX_UNSPECIFIED;
}

TMX_LAYER_TYPE
tmxParseLayerType(const char *value, TMX_BOOL infinite)
{
    if (STREQL(value, "layer") || STREQL(value, "tilelayer"))
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

TMX_DRAW_ORDER
tmxParseDrawOrder(const char *value)
{
    if (STREQL(value, "index"))
        return TMX_DRAW_INDEX;
    if (!STREQL(value, "topdown"))
        tmxErrorInvalidEnum("draw order", value);

    return TMX_DRAW_TOPDOWN;
}

TMX_ALIGN
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

    tmxErrorInvalidEnum(TMX_WORD_HALIGN, value);
    return TMX_ALIGN_LEFT;
}

TMX_ALIGN
tmxParseAlignV(const char *value)
{
    if (STREQL(value, "top"))
        return TMX_ALIGN_TOP;
    if (STREQL(value, "bottom"))
        return TMX_ALIGN_BOTTOM;
    if (STREQL(value, "center"))
        return TMX_ALIGN_CENTER_V;

    tmxErrorInvalidEnum(TMX_WORD_VALIGN, value);
    return TMX_ALIGN_LEFT;
}

TMX_ALIGN
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
        tmxErrorInvalidEnum(WORD_OBJECT_ALIGN, value);

    return TMX_ALIGN_NONE;
}

TMX_RENDER_SIZE
tmxParseRenderSize(const char *value)
{
    if (STREQL(value, "tile"))
        return TMX_RENDER_SIZE_TILE;
    if (STREQL(value, "grid"))
        return TMX_RENDER_SIZE_GRID;

    tmxErrorInvalidEnum(WORD_TILE_RENDER_SIZE, value);
    return TMX_RENDER_SIZE_TILE;
}

TMX_FILL_MODE
tmxParseFillMode(const char *value)
{
    if (STREQL(value, "stretch"))
        return TMX_FILL_MODE_STRETCH;
    if (STREQL(value, "preserve-aspect-fit"))
        return TMX_FILL_MODE_PRESERVE;

    tmxErrorInvalidEnum(WORD_FILL_MODE, value);
    return TMX_FILL_MODE_STRETCH;
}

TMX_ENCODING
tmxParseEncoding(const char *value)
{
    if (STREQL(value, "base64"))
        return TMX_ENCODING_BASE64;
    if (STREQL(value, "csv"))
        return TMX_ENCODING_CSV;

    if (!STREQL(value, "none"))
        tmxErrorInvalidEnum(TMX_WORD_ENCODING, value);
    return TMX_ENCODING_NONE;
}

TMX_COMPRESSION
tmxParseCompression(const char *value)
{
    if (STREQL(value, "gzip"))
        return TMX_COMPRESSION_GZIP;
    if (STREQL(value, "zlib"))
        return TMX_COMPRESSION_ZLIB;
    if (STREQL(value, "zstd"))
        return TMX_COMPRESSION_ZSTD;

    if (!STREQL(value, "none"))
        tmxErrorInvalidEnum(TMX_WORD_COMPRESSION, value);
    return TMX_COMPRESSION_NONE;
}

#pragma endregion

void tmxTilesetConfigureDefaults(TMXtileset *tileset, TMXmap *map)
{
    if (!map)
        return;

    if (!tileset->version && map->version)
        tileset->version = tmxStringDup(map->version);
    if (!tileset->tiled_version && map->tiled_version)
        tileset->tiled_version = tmxStringDup(map->tiled_version);

    // Set defaults for object alignment depending on map orientation if was not specified
    if (tileset->object_align == TMX_ALIGN_NONE)
    {
        if (map->orientation == TMX_ORIENTATION_ORTHOGONAL)
            tileset->object_align = (TMX_ALIGN_BOTTOM | TMX_ALIGN_LEFT);
        else if (map->orientation == TMX_ORIENTATION_ISOMETRIC)
            tileset->object_align = TMX_ALIGN_BOTTOM;
    }
}

void
tmxInitTilesetTiles(TMXtileset *tileset, TMX_BOOL isCollection)
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
            tileset->tiles[i].rect = (TMXrect){.x = x, .y = y, .w = tileset->tile_size.w, .h = tileset->tile_size.h};
        }
    }
}

static TMX_INLINE TMX_FORMAT
tmxDetectFormat(const char *text)
{
    if (!text)
        return TMX_FORMAT_AUTO;

    // This obviously is going to fail if the document if hand-edited, such as a comment:
    //   # JSON comment with <brackets>!
    // This with file-extension detection is as robust of a an implementation as it is going to get for the scope of this project.

    char c;
    for (c = *text; c; c = *(++text))
    {
        if (isspace(c))
            continue;
        if (c == '<')
            return TMX_FORMAT_XML;
        if (c == '{')
            return TMX_FORMAT_JSON;
    }

    tmxErrorMessage(TMX_ERR_WARN, "Unable to detect format.");
    return TMX_FORMAT_XML;
}

static TMX_INLINE uint8_t *
tmxTextConsumeBOM(uint8_t *text)
{
    // UTF-8
    if (text[0] == 0xEF && text[1] == 0xBB && text[2] == 0xBF)
        return text + 3;

    // UTF-32 LE
    if (text[0] == 0xFF && text[1] == 0xFE && text[2] == 0x00 && text[3] == 0x00)
        return text + 4;

    // UTF-32 BE
    if (text[0] == 0x00 && text[1] == 0x00 && text[2] == 0xFE && text[3] == 0xFF)
        return text + 4;

    // UTF-16 LE
    if (text[0] == 0xFF && text[1] == 0xFE)
        return text + 2;

    // UTF-16 BE
    if (text[0] == 0xFE && text[1] == 0xFF)
        return text + 2;

    return text;
}

static void
tmxContextInit(TMXcontext *context, const char *text, const char *filename, TMXcache *cache)
{
    memset(context, 0, sizeof(TMXcontext));
    context->cache = cache;

    if (filename)
    {
        context->basePath  = filename;
        context->constText = tmxFileRead(filename, context->basePath);
        context->freeText  = TMX_TRUE;
    }
    else
    {
        context->constText = text;
        context->freeText  = TMX_FALSE;
    }

    TMX_ASSERT(context->constText);
    context->text = (char *) tmxTextConsumeBOM((uint8_t *) context->constText);
}

static void
tmxContextDeinit(TMXcontext *context)
{
    if (context->freeText)
        tmxFree((void *) context->constText);
}

static TMX_INLINE TMXmap *
tmxParseMapImpl(const char *text, const char *filename, TMXcache *cache, TMX_FORMAT format)
{
    TMXmap *map;
    TMXcontext context;

    tmxContextInit(&context, text, filename, cache);
    switch (format)
    {
        case TMX_FORMAT_JSON: map = tmxParseMapJson(&context); break;
        case TMX_FORMAT_XML: map = tmxParseMapXml(&context); break;
        default:
            tmxErrorMessage(TMX_ERR_PARAM, "Unknown document format.");
            map = NULL;
            break;
    }
    tmxContextDeinit(&context);

    return map;
}

TMXmap *
tmxParseMap(const char *text, TMXcache *cache, TMX_FORMAT format)
{
    if (!text)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxDetectFormat(text);

    return tmxParseMapImpl(text, NULL, cache, format);
}

TMXmap *
tmxLoadMap(const char *filename, TMXcache *cache, TMX_FORMAT format)
{
    if (!filename)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    const char *ext;
    size_t extSize;
    if (format == TMX_FORMAT_AUTO && cwk_path_get_extension(filename, &ext, &extSize))
    {
        if (STREQL(ext, ".tmx") || STREQL(ext, ".xml"))
            format = TMX_FORMAT_XML;
        if (STREQL(ext, ".tmj") || STREQL(ext, ".json"))
            format = TMX_FORMAT_JSON;
    }

    return tmxParseMapImpl(NULL, filename, cache, format);
}

static TMX_INLINE TMXtileset *
tmxParseTilesetImpl(const char *text, const char *filename, TMXcache *cache, TMX_FORMAT format)
{
    TMXtileset *tileset;
    TMXcontext context;

    if (cache && filename && tmxCacheTryGetTileset(cache, filename, &tileset))
        return tileset;

    tmxContextInit(&context, text, filename, cache);
    switch (format)
    {
        case TMX_FORMAT_JSON: tileset = tmxParseTilesetJson(&context); break;
        case TMX_FORMAT_XML: tileset = tmxParseTilesetXml(&context); break;
        default:
            tmxErrorMessage(TMX_ERR_PARAM, "Unknown document format.");
            tileset = NULL;
            break;
    }
    tmxContextDeinit(&context);

    if (tileset)
    {
        if (cache && filename)
            tmxCacheAddTileset(cache, filename, tileset);

        if (!TMX_FLAG(tileset->flags, TMX_FLAG_EXTERNAL))
            tileset->flags |= TMX_FLAG_EMBEDDED;
    }

    return tileset;
}

TMXtileset *
tmxParseTileset(const char *text, TMXcache *cache, TMX_FORMAT format)
{
    if (!text)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxDetectFormat(text);

    return tmxParseTilesetImpl(text, NULL, cache, format);
}

TMXtileset *
tmxLoadTileset(const char *filename, TMXcache *cache, TMX_FORMAT format)
{
    if (!filename)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    const char *ext;
    size_t extSize;
    if (format == TMX_FORMAT_AUTO && cwk_path_get_extension(filename, &ext, &extSize))
    {
        if (STREQL(ext, ".tsx") || STREQL(ext, ".xml"))
            format = TMX_FORMAT_XML;
        if (STREQL(ext, ".tsj") || STREQL(ext, ".json"))
            format = TMX_FORMAT_JSON;
    }

    return tmxParseTilesetImpl(NULL, filename, cache, format);
}

static TMX_INLINE TMXtemplate *
tmxParseTemplateImpl(const char *text, const char *filename, TMXcache *cache, TMX_FORMAT format)
{
    TMXtemplate *template;
    TMXcontext context;

    if (cache && filename && tmxCacheTryGetTemplate(cache, filename, &template))
        return template;

    tmxContextInit(&context, text, filename, cache);
    switch (format)
    {
        case TMX_FORMAT_JSON: template = tmxParseTemplateJson(&context); break;
        case TMX_FORMAT_XML: template = tmxParseTemplateXml(&context); break;
        default:
            tmxErrorMessage(TMX_ERR_PARAM, "Unknown document format.");
            template = NULL;
            break;
    }
    tmxContextDeinit(&context);

    if (template && cache && filename)
        tmxCacheAddTemplate(cache, filename, template);
    return template;
}

TMXtemplate *
tmxParseTemplate(const char *text, TMXcache *cache, TMX_FORMAT format)
{
    if (!text)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxDetectFormat(text);

    return tmxParseTemplateImpl(text, NULL, cache, format);
}

TMXtemplate *
tmxLoadTemplate(const char *filename, TMXcache *cache, TMX_FORMAT format)
{
    if (!filename)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    const char *ext;
    size_t extSize;
    if (format == TMX_FORMAT_AUTO && cwk_path_get_extension(filename, &ext, &extSize))
    {
        if (STREQL(ext, ".tx") || STREQL(ext, ".xml"))
            format = TMX_FORMAT_XML;
        if (STREQL(ext, ".tj") || STREQL(ext, ".json"))
            format = TMX_FORMAT_JSON;
    }

    return tmxParseTemplateImpl(NULL, filename, cache, format);
}