#ifndef TMX_PARSE_H
#define TMX_PARSE_H

#include "tmx.h"
#include "tmx/file.h"
#include "tmx/xml.h"
#include "words.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Contains the state of a document parsing operation.
 */
typedef struct TMXcontext
{
    union
    {
        TMXxmlreader *xml; /** An XML reader state. */
        void *json;        /** A pointer to the head JSON object. */
    };
    const char *basePath;  /** The base path for any child object paths. */
    TMXcache *cache;       /** An optional cache object. */
    TMXmap *map;           /** An optional parent map for this object. */
    char *text;            /** The text pointer positioned after the BOM, if present. */
    const char *constText; /** The original text pointer. */
    TMXbool freeText;      /** Flag indicating if the text is owned by this context and needs freed with it. */
} TMXcontext;

/**
 * @brief Parses a map from the specified @a context in XML format.
 *
 * @param[in] context The context to parse.
 * @return The map object, or @c NULL if an error occurred.
 */
TMXmap *tmxParseMapXml(TMXcontext *context);

/**
 * @brief Parses a map from the specified @a context in JSON format.
 *
 * @param[in] context The context to parse.
 * @return The map object, or @c NULL if an error occurred.
 */
TMXmap *tmxParseMapJson(TMXcontext *context);

/**
 * @brief Parses a tileset from the specified @a context in XML format.
 *
 * @param[in] context The context to parse.
 * @return The tileset object, or @c NULL if an error occurred.
 */
TMXtileset *tmxParseTilesetXml(TMXcontext *context);

/**
 * @brief Parses a tileset from the specified @a context in JSON format.
 *
 * @param[in] context The context to parse.
 * @return The tileset object, or @c NULL if an error occurred.
 */
TMXtileset *tmxParseTilesetJson(TMXcontext *context);

/**
 * @brief Parses a template from the specified @a context in XML format.
 *
 * @param[in] context The context to parse.
 * @return The template object, or @c NULL if an error occurred.
 */
TMXtemplate *tmxParseTemplateXml(TMXcontext *context);

/**
 * @brief Parses a template from the specified @a context in JSON format.
 *
 * @param[in] context The context to parse.
 * @return The template object, or @c NULL if an error occurred.
 */
TMXtemplate *tmxParseTemplateJson(TMXcontext *context);

/**
 * @brief Compares two null-terminated strings for equality.
 *
 * @param[in] a First string to compare.
 * @param[in] b Second string to compare.
 * @return Boolean value indicating if @a a and @a b are equal.
 */
#define STREQL(a, b) (strcmp((a), (b)) == 0)

/**
 * @brief Parse a string as an integer.
 * @param[in] x The string to parse.
 * @return The parsed value.
 */
#define tmxParseInt(x) atoi(x)

/**
 * @brief Parse s string as an unsigned integer.
 * @param[in] x The string to parse.
 * @return The parsed value.
 */
#define tmxParseUint(x) strtoul(x, NULL, 10)

/**
 * @brief Parse a string as a float.
 * @param[in] x The string to parse.
 * @return The parsed value.
 */
#define tmxParseFloat(x) ((float) atof(x))

/**
 * @brief Parses a HTML-style color into a a color structure.
 * @param[in] str The string to parse.
 * @return The parsed color.
 */
TMX_COLOR_T tmxParseColor(const char *str);

/**
 * @brief Parses a string as a boolean.
 * @param[in] str The input string to convert.
 * @return @ref TMX_FALSE when @a str is either "0" or "false", otherwise @ref TMX_TRUE.
 */
#define tmxParseBool(str) (strcmp((str), TMX_WORD_0) != 0 && strcmp((str), TMX_WORD_FALSE) != 0)

/**
 * @brief Parses a property "type" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParsePropertyType(const char *value);

/**
 * @brief Parses an "orientation" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseOrientation(const char *value);

/**
 * @brief Parses a "renderorder" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseRenderOrder(const char *value);

/**
 * @brief Parses a "staggeraxis" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseStaggerAxis(const char *value);

/**
 * @brief Parses a "staggerindex" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseStaggerIndex(const char *value);

/**
 * @brief Parses the name of a layer type into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @param[in] infinite Flag indicating if map is infinite. Used to differentiate tile/chunk layers.
 * @return The enumeration value.
 */
TMXenum tmxParseLayerType(const char *value, TMXbool infinite);

/**
 * @brief Parses a "draworder" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseDrawOrder(const char *value);

/**
 * @brief Parses an "halign" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXflag tmxParseAlignH(const char *value);

/**
 * @brief Parses a "valign" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXflag tmxParseAlignV(const char *value);

/**
 * @brief Parses an "objectalignment" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseObjectAlignment(const char *value);

/**
 * @brief Parses a "tilerendersize" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseRenderSize(const char *value);

/**
 * @brief Parses a "fillmode" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseFillMode(const char *value);

/**
 * @brief Parses an "encoding" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseEncoding(const char *value);

/**
 * @brief Parses a "compression" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMXenum tmxParseCompression(const char *value);

/**
 * @brief Initializes the tiles of a tilesets, allocating memory and initializing defaults.
 *
 * @param[in] tileset The tileset with tiles to initialize.
 * @param[in] isCollection Flag indicating if tileset is an image collection or traditional grid based on single image.
 */
void tmxInitTilesetTiles(TMXtileset *tileset, TMXbool isCollection);

#endif /* TMX_PARSE_H */