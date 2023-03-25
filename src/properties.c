#include "tmx/properties.h"
#include "internal.h"
#include "tmx/types.h"
#include "tmx/xml.h"


TMXbool
tmxTryGetProperty(const TMXproperties *properties, const char *name, TMXproperty **property)
{
    if (!properties || !name)
        return TMX_FALSE;

    struct TMXproperties *entry = NULL;
    HASH_FIND(hh, properties, name, strlen(name), entry);
    if (entry)
    {
        if (*property)
            *property = &entry->value;
        return TMX_TRUE;
    }

    if (*property)
        *property = NULL;
    return TMX_FALSE;
}

TMXproperty *
tmxGetProperty(const TMXproperties *properties, const char *name)
{
    TMXproperty *property = NULL;
    return tmxTryGetProperty(properties, name, &property) ? property : NULL;
}

size_t
tmxGetPropertyCount(const TMXproperties *properties)
{
    if (!properties)
        return 0;
    return (size_t) HASH_COUNT(properties);
}

TMXproperty *
tmxGetPropertiesHead(const TMXproperties *properties)
{
    return properties ? (TMXproperty *) &properties->value : NULL;
}



TMXproperties *
tmxPropertiesDup(TMXproperties *properties)
{
    if (!properties)
        return NULL;

    TMXproperties *result = NULL;
    TMXproperties *temp, *src, *dst;
    size_t keyLen;

    HASH_ITER(hh, properties, src, temp)
    {
        dst    = tmxCalloc(1, sizeof(TMXproperties));
        keyLen = strlen(src->key);

        dst->value.name        = tmxStringCopy(src->value.name, keyLen);
        dst->value.custom_type = tmxStringDup(src->value.custom_type);
        dst->value.type        = src->value.type;

        switch (src->value.type)
        {
            case TMX_UNSPECIFIED:
            case TMX_PROPERTY_FILE:
            case TMX_PROPERTY_STRING: dst->value.value.string = tmxStringDup(src->value.value.string); break;
            case TMX_PROPERTY_INTEGER:
            case TMX_PROPERTY_BOOL:
            case TMX_PROPERTY_OBJECT: dst->value.value.integer = src->value.value.integer; break;
            case TMX_PROPERTY_FLOAT: dst->value.value.decimal = src->value.value.decimal; break;
            case TMX_PROPERTY_COLOR: dst->value.value.color = src->value.value.color; break;
            case TMX_PROPERTY_CLASS: dst->value.value.properties = tmxPropertiesDup(src->value.value.properties);
        }

        dst->key = dst->value.name;
        HASH_ADD_KEYPTR(hh, result, dst->key, keyLen, dst);
    }

    return result;
}



