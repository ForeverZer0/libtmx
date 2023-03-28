#ifndef TMX_COMMON_H
#define TMX_COMMON_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#define TMX_EXPORT __declspec(dllexport)
#define TMX_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
#define TMX_EXPORT __attribute__((visibility("default")))
#define TMX_IMPORT __attribute__((visibility("default")))
#else
#define TMX_EXPORT
#define TMX_IMPORT
#endif

#if defined(TMX_SHARED)
#if defined(TMX_EXPORTS)
#define TMX_PUBLIC TMX_EXPORT
#else
#define TMX_PUBLIC TMX_IMPORT
#endif
#else
#define TMX_PUBLIC
#endif

#ifndef TMX_ASSERT
#include <assert.h>
#define TMX_ASSERT(expr) assert(expr)
#endif

#ifdef TMX_VECTOR_COLOR
#define TMX_COLOR_T TMXcolorf
#else
#define TMX_COLOR_T TMXcolor
#endif

#ifndef TMX_BOOL_T
#define TMX_BOOL_T int /** The integral type used for booleans. */
#endif

#ifndef TMX_INLINE
#define TMX_INLINE __inline__
#endif

#define TMX_MAX_PATH    260 /** The hard-limit for the length of a filesystem path. */
#define TMX_MAX_ERR_MSG 256 /** Maximum length for error message strings, including the null-terminator. */

/**
 * @brief Tests for the presence of a flag in a bitfield.
 * @param value The value to test.
 * @param flag The flag to test the presence of.
 * @return A boolean result.
 */
#define TMX_FLAG(value, flag) (((value) & (flag)) != 0)

/**
 * @brief Shut up, compiler.
 */
#define TMX_UNUSED(x) ((void) x)

/**
 * @brief Returns the minumum of two comparable values.
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return The minimum of the two values.
 * @warning Be wary of possible double-evaluation for the inputs.
 * @note This macro is not part of the "official" API, but is included publicly for convenience.
 */
#define TMX_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Returns the maximum of two comparable values.
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return The minimum of the two values.
 * @warning Be wary of possible double-evaluation for the inputs.
 * @note This macro is not part of the "official" API, but is included publicly for convenience.
 */
#define TMX_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Returns a value clamped between a minimum and maximum.
 * @param x The value to clamp.
 * @param min The minimum value.
 * @param max The maximum value.
 * @return The clamped value.
 * @warning Be wary of possible double-evaluation for the inputs.
 * @note This macro is not part of the "official" API, but is included publicly for convenience.
 */
#define TMX_CLAMP(x, min, max) TMX_MAX(min, TMX_MIN(max, x))

/**
 * @brief Swaps the endianness of a 32-bit value.
 * @param[in] x A 32-bit value to swap the endian of.
 * @return The value with the endian swapped.
 */
#define TMX_ENDIAN_SWAP(x) (((x) >> 24) | (((x) & 0x00FF0000U) >> 8) | (((x) &0x0000FF00U) << 8) | ((x) << 24))

/**
 * @brief A pointer-sized union containing a user-defined value.
 */
typedef union TMXuserptr
{
    void *ptr;    /** The value as a pointer. */
    int32_t id;   /** The value as a 32-bit signed integer. */
    uint32_t uid; /** The value as a 32-bit unsigned integer. */
} TMXuserptr;

#endif /* TMX_COMMON_H */