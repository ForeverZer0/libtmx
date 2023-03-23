#ifndef TMX_PROPERTIES_H
#define TMX_PROPERTIES_H

#include "types.h"

/**
 * @brief Retrieves a property by its name.
 * 
 * @param[in] properties The properties instance to query.
 * @param[in] name The name of the property to retrieve.
 * @param[out] property A pointer that will be assigned the property value within the loop.
 * 
 * @return @ref TMX_TRUE if property was found, otherwise @ref TMX_FALSE. When true, @a property will contain
 * the value, otherwise it will be @c NULL.
*/
TMXbool tmxTryGetProperty(const TMXproperties *properties, const char *name, TMXproperty **property);

/**
 * @brief Retrieves a property by its name.
 * 
 * @param[in] properties The properties instance to query.
 * @param[in] name The name of the property to retrieve.
 * 
 * @return The property with the specified @a name, or @c NULL if none was found.
*/
TMXproperty *tmxGetProperty(const TMXproperties *properties, const char *name);

/**
 * @brief Retrieves the number of property objects stored in the hash.
 * 
 * @param[in] properties The properties instance to query.
 * @return The number of property objects in the hash.
*/
size_t tmxGetPropertyCount(const TMXproperties *properties);

/**
 * @brief Retrieves the first property in the properties hash.
 * 
 * @param[in] properties The properties instance to query.
 * @return The first property value, or @c NULL if empty.
*/
TMXproperty *tmxGetPropertiesHead(const TMXproperties *properties);

/**
 * @brief Adds a property value to the hash.
 * 
 * @param[in] properties The properties instance to add to.
 * @param[in] property The property to add, whose name must not exist in the hash.
 * 
 * @return @ref TMX_TRUE if @a property was successfully added, otherwise @ref TMX_FALSE.
*/
TMXbool tmxPropertiesAdd(TMXproperties *properties, TMXproperty *property);

/**
 * @brief Removes a property value from the hash.
 * 
 * @param[in] properties The properties instance to remove from.
 * @param[in] name The name of the property to remove.
 * 
 * @return @ref TMX_TRUE if a property with @a name was found and successfully removed, otherwise @ref TMX_FALSE.
*/
TMXbool tmxPropertiesRemove(TMXproperties *properties, const char *name);

/**
 * @brief Frees a properties object.
 * 
 * @param[in] properties The properties instance to free.
*/
void tmxFreeProperties(TMXproperties *properties);

/**
 * @brief Creates a deep-copy of the properties.
 * 
 * @param[in] properties The properties to duplicate.
 * @return The newly allocated cloned properties.
*/
TMXproperties *tmxPropertiesDup(TMXproperties *properties);

/**
 * @brief Helper macro to iterate properties.
 * 
 * @param[in] properties The properties value to enumerate.
 * @param[in,out] property A pointer that will be assigned the property value within the loop.
*/
#define tmxPropertiesForeach(properties, property)                                                                                         \
    for ((property) = tmxGetPropertiesHead(properties); (property); (property) = (property)->next)

#endif /* TMX_PROPERTIES_H */