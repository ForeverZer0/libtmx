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
    TMX_BOOL freeText;      /** Flag indicating if the text is owned by this context and needs freed with it. */
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
TMX_PROPERTY_TYPE tmxParsePropertyType(const char *value);

/**
 * @brief Parses an "orientation" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_ORIENTATION tmxParseOrientation(const char *value);

/**
 * @brief Parses a "renderorder" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_RENDER_ORDER tmxParseRenderOrder(const char *value);

/**
 * @brief Parses a "staggeraxis" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_STAGGER_AXIS tmxParseStaggerAxis(const char *value);

/**
 * @brief Parses a "staggerindex" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_STAGGER_INDEX tmxParseStaggerIndex(const char *value);

/**
 * @brief Parses the name of a layer type into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @param[in] infinite Flag indicating if map is infinite. Used to differentiate tile/chunk layers.
 * @return The enumeration value.
 */
TMX_LAYER_TYPE tmxParseLayerType(const char *value, TMX_BOOL infinite);

/**
 * @brief Parses a "draworder" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_DRAW_ORDER tmxParseDrawOrder(const char *value);

/**
 * @brief Parses an "halign" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_ALIGN tmxParseAlignH(const char *value);

/**
 * @brief Parses a "valign" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_ALIGN tmxParseAlignV(const char *value);

/**
 * @brief Parses an "objectalignment" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_ALIGN tmxParseObjectAlignment(const char *value);

/**
 * @brief Parses a "tilerendersize" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_RENDER_SIZE tmxParseRenderSize(const char *value);

/**
 * @brief Parses a "fillmode" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_FILL_MODE tmxParseFillMode(const char *value);

/**
 * @brief Parses an "encoding" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_ENCODING tmxParseEncoding(const char *value);

/**
 * @brief Parses a "compression" value into an enumeration.
 * @param[in] value The null-terminated string to parse.
 * @return The enumeration value.
 */
TMX_COMPRESSION tmxParseCompression(const char *value);

/**
 * @brief Initializes the tiles of a tilesets, allocating memory and initializing defaults.
 *
 * @param[in] tileset The tileset with tiles to initialize.
 * @param[in] isCollection Flag indicating if tileset is an image collection or traditional grid based on single image.
 */
void tmxInitTilesetTiles(TMXtileset *tileset, TMX_BOOL isCollection);

/**
 * @brief Updates tileset values that are often unspecified and have defaults that depend on map.
 * 
 * @param[in] tileset The tileset to configure.
 * @param[in] map The map instance, or @c NULL.
 */
void tmxTilesetConfigureDefaults(TMXtileset *tileset, TMXmap *map);

/**
 * @brief Retrieves the number of values in the CSV-encoded @a input string.
 *
 * @param[in] input The CSV-encoded string to query.
 * @param[in] inputSize The size of the @a input, in bytes.
 *
 * @return The number of elements defined in the @a input string.
 */
size_t tmxCsvCount(const char *input, size_t inputSize);

/**
 * @brief Decodes a CSV-encoded string of tile IDs into an array.
 *
 * @param[in] input The CSV-encoded string to decode.
 * @param[in] inputSize The size of the @a input string, in bytes.
 * @param[in,out] output A pointer to array of tile IDs to receive the output.
 * @param[in] outputCount The maximum number of tile IDs that can be written to the @a output array.
 *
 * @return The number of tile IDs written to the @a output array.
 */
size_t tmxCsvDecode(const char *input, size_t inputSize, TMXgid *output, size_t outputCount);

/**
 * @brief Takes a Base64-encoded string and decodes and decompresses it to an @a output buffer.
 *
 * @param[in] input The Base64-encoded buffer to decode and (optionally) decompress.
 * @param[in] inputSize The size of the @a input buffer, in bytes.
 * @param[in,out] output A buffer allocated with sufficient size to receive the decompressed tile data.
 * @param[in] outputCount The maximum number of elements that can be written to the @a output array.
 * @param[in] compression A enumeration value indicating the type of compression used, if any.
 *
 * @return The number of tile IDs written to the @a output buffer.
 */
size_t tmxInflate(const char *input, size_t inputSize, TMXgid *output, size_t outputCount, TMX_COMPRESSION compression);

#endif /* TMX_PARSE_H */