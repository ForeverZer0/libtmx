/**
 * @file internal.h
 * @brief Contains common helper macros and functions that are not part of the public API.
 */
#ifndef TMX_UTILS_H
#define TMX_UTILS_H

#include "tmx/error.h"
#include "tmx/types.h"
#include <stdlib.h>
#include <string.h>

#ifndef TMX_ASSERT
#include <assert.h>
#define TMX_ASSERT assert
#endif

typedef struct TMXcontext
{
    const char *baseDir;
    TMXcache *cache;
    TMXmap *map;
} TMXcontext;

#define TMX_CALLOC(type) ((type*)tmxCalloc(1, sizeof(type)))

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
 * @brief Gets the string as a boolean.
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

#endif /* TMX_UTILS_H */