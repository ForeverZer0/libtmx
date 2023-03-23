#ifndef TMX_COMPRESSION_H
#define TMX_COMPRESSION_H

#include "types.h"

/**
 * @brief Swaps the endianness of a 32-bit value.
 * @param[in] x A 32-bit value to swap the endian of.
 * @return The 32-bit value with the endian swapped.
 */
#define TMX_ENDIAN_SWAP(x) (((x) >> 24) | (((x) &0x00FF0000U) >> 8) | (((x) &0x0000FF00U) << 8) | ((x) << 24))

#define TMX_COMPRESSION_NONE 0 /** No compression. */
#define TMX_COMPRESSION_GZIP 1 /** Gzip compression (i.e. DEFLATE) */
#define TMX_COMPRESSION_ZLIB 2 /** Zlib compression (i.e. DEFLATE with additional header and checksum) */
#define TMX_COMPRESSION_ZSTD 3 /** Zstandard compression. Optional compile-time algorithm created by Facebook. */

#define TMX_ENCODING_NONE   0 /** No encoding. */
#define TMX_ENCODING_CSV    1 /** A string containing comma-separated values. */
#define TMX_ENCODING_BASE64 2 /** A Base64-encoded string. */

/**
 * @brief Tests whether the specified @a input is a valid Base64 string.
 *
 * @param[in] input The string to test.
 * @param[in] inputSize The number of bytes in @a input, in bytes.
 *
 * @return @ref TMX_TRUE if @a input is of valid length and characters, otherwise @ref TMX_FALSE.
 */
TMXbool tmxBase64IsValid(const char *input, size_t inputSize);

/**
 * @brief Retrieves the required number of bytes to contain the decoded Base64 @a input.
 *
 * @param[in] input The Base64-encoded string to test.
 * @param[in] inputSize The size of the input string, in bytes.
 *
 * @return The required size of a buffer to contain the decoded output.
 */
size_t tmxBase64DecodedSize(const char *input, size_t inputSize);

/**
 * @brief Decodes a Base64-encoded string into an @a output buffer.
 *
 * @param[in] input The Base64-encoded string to decode.
 * @param[in] inputSize The size of the @a input string, in bytes.
 * @param[in,out] output A buffer allocated with sufficient size to receive the decoded output.
 * @param[in] outputSize The number of bytes available in the @a output buffer to write to.
 *
 * @return The number of bytes written to the @a output buffer.
 */
size_t tmxBase64Decode(const char *input, size_t inputSize, void *output, size_t outputSize);

/**
 * @brief Inflates a Gzip-compressed block of memory into an @a output buffer.
 *
 * @param[in] input The Gzip-compressed buffer to decompress.
 * @param[in] inputSize The size of the @a input buffer, in bytes.
 * @param[in,out] output A buffer allocated with sufficient size to receive the decompressed output.
 * @param[in] outputSize The number of bytes available in the @a output buffer to write to.
 *
 * @return The number of bytes written to the @a output buffer.
 */
size_t tmxInflateGzip(const void *input, size_t inputSize, void *output, size_t outputSize);

/**
 * @brief Inflates a Zlib-compressed block of memory into an @a output buffer.
 *
 * @param[in] input The Zlib-compressed buffer to decompress.
 * @param[in] inputSize The size of the @a input buffer, in bytes.
 * @param[in,out] output A buffer allocated with sufficient size to receive the decompressed output.
 * @param[in] outputSize The number of bytes available in the @a output buffer to write to.
 *
 * @return The number of bytes written to the @a output buffer.
 */
size_t tmxInflateZlib(const void *input, size_t inputSize, void *output, size_t outputSize);

/**
 * @brief Inflates a Zstd-compressed block of memory into an @a output buffer.
 *
 * @param[in] input The Zstd-compressed buffer to decompress.
 * @param[in] inputSize The size of the @a input buffer, in bytes.
 * @param[in,out] output A buffer allocated with sufficient size to receive the decompressed output.
 * @param[in] outputSize The number of bytes available in the @a output buffer to write to.
 *
 * @return The number of bytes written to the @a output buffer.
 * @note Zstandard support is a compile-time option that must be enabled by defining the @c TMX_WITH_ZSTD compiler flag. When not
 * present, this function will simply emit a @ref TMX_ERR_UNSUPPORTED error and return.
 */
size_t tmxInflateZstd(const void *input, size_t inputSize, void *output, size_t outputSize);

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
size_t tmxInflate(const char *input, size_t inputSize, TMXgid *output, size_t outputCount, TMXenum compression);

#endif /* TMX_COMPRESSION_H */