#include "internal.h"

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

static TMXproperties *
tmxPropertyDup(TMXproperties *src)
{
    TMXproperties *dst = tmxCalloc(1, sizeof(TMXproperties));
    size_t keyLen      = strlen(src->key);

    dst->value.name  = tmxStringCopy(src->value.name, keyLen);
    dst->value.class = tmxStringDup(src->value.class);
    dst->value.type  = src->value.type;

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
    return dst;
}

TMXproperties *
tmxPropertiesDup(TMXproperties *properties)
{
    if (!properties)
        return NULL;

    TMXproperties *result = NULL;
    TMXproperties *temp, *src, *dst;

    HASH_ITER(hh, properties, src, temp)
    {
        dst = tmxPropertyDup(src);
        HASH_ADD_KEYPTR(hh, result, dst->key, strlen(dst->key), dst);
    }

    return result;
}

TMXproperties *
tmxPropertiesMerge(TMXproperties *dst, TMXproperties *src)
{
    // Nothing to add, early-out
    if (!src)
        return dst;

    // Nothing to merge to, create copy of all properties
    if (!dst)
        return tmxPropertiesDup(src);

    // Enumerate through source properties
    TMXproperties *srcProp, *tmpProp, *dstProp;
    HASH_ITER(hh, src, srcProp, tmpProp)
    {
        // Skip if this key already exists within the destination hash
        dstProp = NULL;
        HASH_FIND(hh, dst, src->key, strlen(src->key), dstProp);
        if (dstProp)
            continue;

        // Create a copy and add it to the destination hash
        dstProp = tmxPropertiesDup(srcProp);
        HASH_ADD_KEYPTR(hh, dst, dstProp->key, strlen(dstProp->key), dstProp);
    }

    return dst;
}