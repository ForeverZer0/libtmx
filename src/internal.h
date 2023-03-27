/**
 * @file internal.h
 * @brief Contains common helper macros and functions that are not part of the public API.
 */
#ifndef TMX_UTILS_H
#define TMX_UTILS_H

#include "tmx/compression.h"
#include "tmx/error.h"
#include "tmx/types.h"
#include <stdlib.h>
#include <string.h>

#ifndef TMX_ASSERT
#include <assert.h>
#define TMX_ASSERT(expr) assert(expr)
#endif

#include "tmx/memory.h"
#define uthash_malloc(sz)    tmxMalloc(sz)
#define uthash_free(ptr, sz) tmxFree(ptr)
#include "uthash.h"

struct TMXproperties
{
    const char *key;
    TMXproperty value;
    UT_hash_handle hh;
};

#define TMX_ALLOC(type) ((type *) tmxCalloc(1, sizeof(type)))

/**
 * @brief Compares two null-terminated strings for equality.
 *
 * @param[in] a First string to compare.
 * @param[in] b Second string to compare.
 * @return Boolean value indicating if @a a and @a b are equal.
 */
#define STREQL(a, b) (strcmp((a), (b)) == 0)

/**
 * @brief Returns the minumum of two comparable values.
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return The minimum of the two values.
 * @warning Be wary of possible double-evaluation for the inputs.
 */
#define TMX_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Returns the maximum of two comparable values.
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return The minimum of the two values.
 * @warning Be wary of possible double-evaluation for the inputs.
 */
#define TMX_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Returns a value clamped between a minimum and maximum.
 * @param x The value to clamp.
 * @param min The minimum value.
 * @param max The maximum value.
 * @return The clamped value.
 * @warning Be wary of possible double-evaluation for the inputs.
 */
#define TMX_CLAMP(x, min, max) TMX_MAX(min, TMX_MIN(max, x))

/**
 * @brief Tests for the presence of a flag in a bitfield.
 * @param value The value to test.
 * @param flag The flag to test the presence of.
 * @return A boolean result.
 */
#define TMX_FLAG(value, flag) (((value) & (flag)) != 0)

/**
 * @brief Make your compiler be quiet.
 */
#define TMX_UNUSED(x) ((void) x)

/**
 * @brief Conversion of string to integer.
 */
#define TMX_STR2INT(x) atoi(x)

/**
 * @brief Conversion of string to float.
 */
#define TMX_STR2FLT(x) ((float) atof(x))

/**
 * @brief Conversion of string to a boolean value.
 */
#define TMX_STR2BOOL(x) tmxStringBool(x)

/**
 * @brief Parses a color in the HTML-style used by Tiled to a structure.
 * @param[in] str The string to parse.
 * @return The parsed color.
 */
TMX_COLOR_T tmxParseColor(const char *str);

/**
 * Allocates and copies a string.
 * @param[in] input The string to duplicate.
 * @param[in] inputSize The length of the @a input string to copy.
 * @return A duplicate of the string, or @c NULL when @a input is @c NULL.
 * @note The returned result will be null-terminated.
 */
char *tmxStringCopy(const char *input, size_t inputSize);

/**
 * @brief Parses a string as a boolean. Supports numeric strings (i.e. "0", "1") or words (i.e. "true", "false").
 * @param[in] str The input string to convert.
 * @return The boolean result.
 */
TMXbool tmxStringBool(const char *str);

/**
 * @brief Allocates and copies a zero-terminated string.
 * @param[in] input The string to duplicate.
 * @return A duplicate of the string, or @c NULL when @a input is @c NULL.
 * @note The returned result will be null-terminated.
 */
#define tmxStringDup(input) tmxStringCopy(input, 0)

/**
 * @brief Emits an error for an invalid enumeration value.
 * @param enumName The name of the enumeration as a string literal.
 * @param value The value that was given.
 */
#define tmxErrorUnknownEnum(enumName, value)                                                                                               \
    tmxErrorFormat(TMX_ERR_VALUE, "Unrecognized " enumName " \"%s\" specified.", value ? value : "")

/**
 * @brief Invokes the user-callback for image loading, if set.
 *
 * @param[in] image The image that is loaded.
 * @param[in] basePath An optional base path that the image source is relative.
 */
void tmxImageUserLoad(TMXimage *image, const char *basePath);

/**
 * @brief Invokes the user-callback to free an image, if set.
 *
 * @param[in] image The image to free.
 */
void tmxImageUserFree(TMXimage *image);

/**
 * @brief Returns the size of of subsection that makes up the directory portion of the given @a path.
 *
 * @param[in] path The filesystem path to query.
 * @return The length of directory portion (including final path separator), or @c 0 if unable to determine.
 */
size_t tmxFileDirectory(const char *path);

/**
 * @brief Builds an absolute path from two paths: a relative path, and the an optional base path it is relative to.
 * 
 * @param[in] path The path to ensure is absolute. May be relative or absolute.
 * @param[in] basePath An optional base path that @a path is relative to.
 * @param[in,out] buffer A buffer to receive the resulting absolute path.
 * @param[in] bufferSize The maximum number of bytes that can be written to the buffer, including the null-terminator.
 * @return The number of bytes written to the @a buffer, excluding the null-terminator.
 * @note The @a buffer is guaranteed to be null-terminated.
 */
size_t tmxFileAbsolutePath(const char *path, const char *basePath, char *buffer, size_t bufferSize); // TODO


char *tmxFileRead(const char *path, const char *basePath); // TODO

#define tmxArrayPush(T, array, object, count, capacity)                                                                                    \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (count >= capacity)                                                                                                             \
        {                                                                                                                                  \
            capacity *= 2;                                                                                                                 \
            array = (T *) tmxRealloc(array, capacity * sizeof(T));                                                                         \
        }                                                                                                                                  \
        array[count++] = object;                                                                                                           \
    } while (0)

#define tmxArrayFinish(T, array, count, capacity)                                                                                          \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (count < capacity)                                                                                                              \
            array = (T *) tmxRealloc(array, count * sizeof(T));                                                                            \
    } while (0)

#pragma region Enumerations

static TMX_INLINE TMXenum
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

static TMX_INLINE TMXenum
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

    tmxErrorUnknownEnum("orientation", value);
    return TMX_UNSPECIFIED;
}

static TMX_INLINE TMXenum
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

    tmxErrorUnknownEnum("render order", value);
    return TMX_UNSPECIFIED;
}

static TMX_INLINE TMXenum
tmxParseStaggerAxis(const char *value)
{
    if (STREQL(value, "x"))
        return TMX_STAGGER_AXIS_X;
    if (STREQL(value, "y"))
        return TMX_STAGGER_AXIS_Y;

    tmxErrorUnknownEnum("stagger axis", value);
    return TMX_UNSPECIFIED;
}

static TMX_INLINE TMXenum
tmxParseStaggerIndex(const char *value)
{
    if (STREQL(value, "even"))
        return TMX_STAGGER_INDEX_EVEN;
    if (STREQL(value, "odd"))
        return TMX_STAGGER_INDEX_ODD;

    tmxErrorUnknownEnum("stagger index", value);
    return TMX_UNSPECIFIED;
}

static TMX_INLINE TMXenum
tmxParseLayerType(const char *value, TMXbool infinite)
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

static TMX_INLINE TMXenum
tmxParseDrawOrder(const char *value)
{
    if (STREQL(value, "index"))
        return TMX_DRAW_INDEX;
    if (!STREQL(value, "topdown"))
        tmxErrorUnknownEnum("draw order", value);

    return TMX_DRAW_TOPDOWN;
}

static TMX_INLINE TMXflag
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

    tmxErrorUnknownEnum("horizontal align", value);
    return TMX_ALIGN_LEFT;
}

static TMX_INLINE TMXflag
tmxParseAlignV(const char *value)
{
    if (STREQL(value, "top"))
        return TMX_ALIGN_TOP;
    if (STREQL(value, "bottom"))
        return TMX_ALIGN_BOTTOM;
    if (STREQL(value, "center"))
        return TMX_ALIGN_CENTER_V;

    tmxErrorUnknownEnum("vertical align", value);
    return TMX_ALIGN_LEFT;
}

static TMX_INLINE TMXenum
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
        tmxErrorUnknownEnum("object alignment", value);

    return TMX_UNSPECIFIED;
}

static TMX_INLINE TMXenum
tmxParseRenderSize(const char *value)
{
    if (STREQL(value, "tile"))
        return TMX_RENDER_SIZE_TILE;
    if (STREQL(value, "grid"))
        return TMX_RENDER_SIZE_GRID;

    tmxErrorUnknownEnum("render size", value);
    return TMX_UNSPECIFIED;
}

static TMX_INLINE TMXenum
tmxParseFillMode(const char *value)
{
    if (STREQL(value, "stretch"))
        return TMX_FILL_MODE_STRETCH;
    if (STREQL(value, "preserve-aspect-fit"))
        return TMX_FILL_MODE_PRESERVE;

    tmxErrorUnknownEnum("fill mode", value);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseEncoding(const char *value)
{
    if (STREQL(value, "base64"))
        return TMX_ENCODING_BASE64;
    if (STREQL(value, "csv"))
        return TMX_ENCODING_CSV;

    if (!STREQL(value, "none"))
        tmxError(TMX_ERR_PARAM);
    return TMX_UNSPECIFIED;
}

static inline TMXenum
tmxParseCompression(const char *value)
{
    if (STREQL(value, "gzip"))
        return TMX_COMPRESSION_GZIP;
    if (STREQL(value, "zlib"))
        return TMX_COMPRESSION_ZLIB;
    if (STREQL(value, "zstd"))
        return TMX_COMPRESSION_ZSTD;

    if (!STREQL(value, "none"))
        tmxError(TMX_ERR_PARAM);
    return TMX_UNSPECIFIED;
}

void tmxObjectMergeTemplate(TMXobject *dst, TMXobject *src);

#pragma endregion

#endif /* TMX_UTILS_H */