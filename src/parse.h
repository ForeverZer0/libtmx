#ifndef TMX_PARSE_H
#define TMX_PARSE_H

#include "tmx.h"
#include "words.h"
#include "tmx/xml.h"

typedef struct TMXcontext
{
    const char *basePath;
    TMXcache *cache;
    TMXmap *map;
    union
    {
        TMXxmlreader *xml;
        void *json;
    };
    char *text;
    const char *constText;
    TMXbool freeText;
} TMXcontext;

TMXmap *tmxParseMapJson(TMXcontext *context);

TMXmap *tmxParseMapXml(TMXcontext *context);

TMXtileset *tmxParseTilesetXml(TMXcontext *context);

TMXtileset *tmxParseTilesetJson(TMXcontext *context);

TMXtemplate *tmxParseTemplateXml(TMXcontext *context);

TMXtemplate *tmxParseTemplateJson(TMXcontext *context);

#endif /* TMX_PARSE_H */