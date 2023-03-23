#include "tmx/properties.h"
#include "internal.h"
#include "tmx/types.h"
#include "tmx/xml.h"
#include "uthash.h"

struct TMXproperties
{
    const char *key;
    TMXproperty value;
    UT_hash_handle hh;
};

TMXbool
tmxTryGetProperty(const TMXproperties *properties, const char *name, TMXproperty **property)
{
    if (!properties || !name || !*property)
        return TMX_FALSE;

    struct TMXproperties *entry = NULL;

    HASH_FIND(hh, properties, name, strlen(name), entry);
    if (entry)
    {
        *property = &entry->value;
        return TMX_TRUE;
    }
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

static inline void
tmxFreeProperty(TMXproperty *property)
{
    tmxFree((void *) property->name);
    tmxFree((void *) property->custom_type);
    switch (property->type)
    {
        case TMX_PROPERTY_STRING:
        case TMX_PROPERTY_FILE: tmxFree((void *) property->value.string); break;
        case TMX_PROPERTY_CLASS: tmxFreeProperties(property->value.properties); break;
    }
    tmxFree(property);
}

void
tmxFreeProperties(TMXproperties *properties)
{
    if (!properties)
        return;

    struct TMXproperties *entry, *temp;
    HASH_ITER(hh, properties, entry, temp)
    {
        HASH_DEL(properties, entry);
        tmxFreeProperty(&entry->value);
        // The key is the same pointer as the property name, so no need to free it
        tmxFree(entry);
    }
    tmxFree(properties);
}

TMXbool
tmxPropertiesAdd(TMXproperties *properties, TMXproperty *property)
{
    if (!property)
        return TMX_FALSE;

    TMXproperty *previous;
    struct TMXproperties *entry = NULL;

    // Check if a property with the same name already exists before adding.
    size_t keyLen = strlen(property->name);
    HASH_FIND(hh, properties, property->name, keyLen, entry);
    if (entry)
        return TMX_FALSE;

    // Allocate a new entry.
    entry        = tmxCalloc(1, sizeof(struct TMXproperties));
    entry->key   = property->name;
    entry->value = *property;

    // Add the property, and set the previous property's "next" field to point to it.
    HASH_ADD_KEYPTR(hh, properties, property->name, keyLen, entry);
    previous = (TMXproperty *) entry->hh.prev;
    if (previous)
        previous->next = property;

    return TMX_TRUE;
}

// TODO

TMXbool
tmxPropertiesRemove(TMXproperties *properties, const char *name)
{
    if (!properties || !name)
        return TMX_FALSE;

    TMXproperties *entry, *previous;

    // Find the entry with the given name.
    size_t keyLen = strlen(name);
    HASH_FIND(hh, properties, name, keyLen, entry);
    if (entry)
        return TMX_FALSE;

    // If found, update the "next" property of the previous entry, and remove it from hash.
    previous = entry->hh.prev;
    if (previous)
        previous->value.next = entry->value.next;
    HASH_DEL(properties, entry);

    // Deallocate memory
    tmxFreeProperty(&entry->value);
    tmxFree(entry);
    return TMX_TRUE;
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

TMXproperties *
tmxXmlReadProperties(TMXxmlreader *xml)
{
    const char *name;
    const char *value;
    size_t size;
    TMXproperties *properties = NULL;

    while (tmxXmlReadElement(xml, &name, &size))
    {
        if (!STREQL(name, "property"))
            continue;

        // TODO: Assert

        TMXproperties *entry  = tmxCalloc(1, sizeof(TMXproperties));
        TMXproperty *property = &entry->value; // TODO
        while (tmxXmlReadAttr(xml, &name, &value))
        {
            if (STREQL(name, "name"))
                property->name = tmxStringDup(value);
            else if (STREQL(name, "type"))
                property->type = tmxParsePropertyType(value);
            else if (STREQL(name, "propertytype"))
                property->custom_type = value;
            else if (STREQL(name, "value"))
            {
                // TODO: Store result and parse after all attributes have been processed?
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
        }

        if (tmxXmlMoveToContent(xml))
            property->value.properties = tmxXmlReadProperties(xml);

        entry->key = property->name;
        HASH_ADD_KEYPTR(hh, properties, entry->key, strlen(entry->key), entry);

        TMXproperties *previous = entry->hh.prev;
        if (previous)
            previous->value.next = property;
    }

    return properties;
}