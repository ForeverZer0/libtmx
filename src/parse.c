#include "parse.h"
#include "cwalk.h"
#include "internal.h"
#include "tmx/cache.h"
#include "tmx/error.h"
#include <ctype.h>

static TMX_INLINE TMXenum
tmxMapFormatFromExtension(const char *filename)
{
    const char *ext;
    size_t extSize;

    if (cwk_path_get_extension(filename, &ext, &extSize))
    {
        if (STREQL(ext, ".tmx") || STREQL(ext, ".xml"))
            return TMX_FORMAT_XML;
        if (STREQL(ext, ".tmj") || STREQL(ext, ".json"))
            return TMX_FORMAT_JSON;
    }
    return TMX_FORMAT_AUTO;
}

static TMX_INLINE TMXenum
tmxTilesetFormatFromExtension(const char *filename)
{
    const char *ext;
    size_t extSize;

    if (cwk_path_get_extension(filename, &ext, &extSize))
    {
        if (STREQL(ext, ".tsx") || STREQL(ext, ".xml"))
            return TMX_FORMAT_XML;
        if (STREQL(ext, ".tsj") || STREQL(ext, ".json"))
            return TMX_FORMAT_JSON;
    }
    return TMX_FORMAT_AUTO;
}

static TMX_INLINE TMXenum
tmxTemplateFormatFromExtension(const char *filename)
{
    const char *ext;
    size_t extSize;

    if (cwk_path_get_extension(filename, &ext, &extSize))
    {
        if (STREQL(ext, ".tx") || STREQL(ext, ".xml"))
            return TMX_FORMAT_XML;
        if (STREQL(ext, ".tj") || STREQL(ext, ".json"))
            return TMX_FORMAT_JSON;
    }
    return TMX_FORMAT_AUTO;
}

static TMX_INLINE TMXenum
tmxFormatFromText(const char *text)
{
    if (!text)
        return TMX_FORMAT_AUTO;

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
tmxParseMapImpl(const char *text, const char *filename, TMXcache *cache, TMXenum format)
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
tmxParseMap(const char *text, TMXcache *cache, TMXenum format)
{
    if (!text)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxFormatFromText(text);

    return tmxParseMapImpl(text, NULL, cache, format);
}

TMXmap *
tmxLoadMap(const char *filename, TMXcache *cache, TMXenum format)
{
    if (!filename)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxMapFormatFromExtension(filename);

    return tmxParseMapImpl(NULL, filename, cache, format);
}

static TMX_INLINE TMXtileset *
tmxParseTilesetImpl(const char *text, const char *filename, TMXcache *cache, TMXenum format)
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

    if (tileset && cache && filename)
        tmxCacheAddTileset(cache, filename, tileset);
    return tileset;
}

TMXtileset *
tmxParseTileset(const char *text, TMXcache *cache, TMXenum format)
{
    if (!text)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxFormatFromText(text);

    return tmxParseTilesetImpl(text, NULL, cache, format);
}

TMXtileset *
tmxLoadTileset(const char *filename, TMXcache *cache, TMXenum format)
{
    if (!filename)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxTilesetFormatFromExtension(filename);

    return tmxParseTilesetImpl(NULL, filename, cache, format);
}

static TMX_INLINE TMXtemplate *
tmxParseTemplateImpl(const char *text, const char *filename, TMXcache *cache, TMXenum format)
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
tmxParseTemplate(const char *text, TMXcache *cache, TMXenum format)
{
    if (!text)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxFormatFromText(text);

    return tmxParseTemplateImpl(text, NULL, cache, format);
}

TMXtemplate *
tmxLoadTemplate(const char *filename, TMXcache *cache, TMXenum format)
{
    if (!filename)
    {
        tmxError(TMX_ERR_VALUE);
        return NULL;
    }

    if (format == TMX_FORMAT_AUTO)
        format = tmxTemplateFormatFromExtension(filename);

    return tmxParseTemplateImpl(NULL, filename, cache, format);
}