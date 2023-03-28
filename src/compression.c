#include "tmx/compression.h"
#include "internal.h"
#include "tmx/memory.h"
#include "parse.h"
#include <string.h>

#define MINIZ_NO_MALLOC
#define MINIZ_NO_STDIO
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_DEFLATE_APIS

#define MZ_MALLOC  tmxMalloc
#define MZ_FREE    tmxFree
#define MZ_REALLOC tmxRealloc
#include "miniz.h"

#define TMX_GZIP_HEADER_SIZE 10

int
tmxBase64IsValid(const char *input, size_t inputSize)
{
    size_t i;
    if (inputSize % 4 != 0)
        return TMX_FALSE;

    char c = *input;
    for (i = 0; i < inputSize; c = input[++i])
    {
        if (c >= '0' && c <= '9')
            continue;
        if (c >= 'A' && c <= 'Z')
            continue;
        if (c >= 'a' && c <= 'z')
            continue;
        if (c == '+' || c == '/' || c == '=')
            continue;
        return TMX_FALSE;
    }
    return TMX_TRUE;
}

size_t
tmxBase64DecodedSize(const char *input, size_t inputSize)
{
    size_t ret;
    size_t i;

    if (input == NULL)
        return 0;

    ret = inputSize / 4 * 3;

    for (i = inputSize; i-- > 0;)
    {
        if (input[i] == '=')
            ret--;
        else
            break;
    }
    return ret;
}

size_t
tmxBase64Decode(const char *input, size_t inputSize, void *output, size_t outputSize)
{
    // https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/

    static const int decodeTable[80] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1,
                                        -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                                        18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31,
                                        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

    size_t i, j, b64Size;
    int v;
    char *outp = output;

    if (input == NULL || outp == NULL)
    {
        tmxErrorMessage(TMX_ERR_VALUE, "Undefined value.");
        return 0;
    }
    if (inputSize % 4 != 0)
    {
        tmxErrorMessage(TMX_ERR_FORMAT, "Invalid length input for Base64, expected factor of 4.");
        return 0;
    }

    b64Size = tmxBase64DecodedSize(input, inputSize);
    if (outputSize < b64Size)
    {
        tmxErrorMessage(TMX_ERR_VALUE, "Output buffer has insufficient size.");
        return 0;
    }

    for (i = 0, j = 0; i < inputSize; i += 4, j += 3)
    {
        v = decodeTable[input[i] - 43];
        v = (v << 6) | decodeTable[input[i + 1] - 43];
        v = input[i + 2] == '=' ? v << 6 : (v << 6) | decodeTable[input[i + 2] - 43];
        v = input[i + 3] == '=' ? v << 6 : (v << 6) | decodeTable[input[i + 3] - 43];

        outp[j] = (v >> 16) & 0xFF;
        if (input[i + 2] != '=')
            outp[j + 1] = (v >> 8) & 0xFF;
        if (input[i + 3] != '=')
            outp[j + 2] = v & 0xFF;
    }

    return b64Size;
}

size_t
tmxInflateGzip(const void *input, size_t inputSize, void *output, size_t outputSize)
{
    if (inputSize <= TMX_GZIP_HEADER_SIZE)
    {
        tmxError(TMX_ERR_FORMAT);
        return 0;
    }
    size_t result = tinfl_decompress_mem_to_mem(output, outputSize, &input[TMX_GZIP_HEADER_SIZE], inputSize - TMX_GZIP_HEADER_SIZE, 0);
    if (result == (size_t) TINFL_STATUS_FAILED)
    {
        tmxError(TMX_ERR_FORMAT);
        return 0;
    }
    return result;
}

size_t
tmxInflateZlib(const void *input, size_t inputSize, void *output, size_t outputSize)
{
    size_t result = tinfl_decompress_mem_to_mem(output, outputSize, input, inputSize, TINFL_FLAG_PARSE_ZLIB_HEADER);
    if (result == (size_t) TINFL_STATUS_FAILED)
    {
        tmxError(TMX_ERR_FORMAT);
        return 0;
    }
    return result;
}

#ifdef TMX_NO_ZSTD

size_t
tmxInflateZstd(const void *input, size_t inputSize, void *output, size_t outputSize)
{
    TMX_UNUSED(input);
    TMX_UNUSED(inputSize);
    TMX_UNUSED(output);
    TMX_UNUSED(outputSize);
    tmxError(TMX_ERR_UNSUPPORTED);
    return 0;
}

#else

size_t ZSTD_decompress(void *dst, size_t dstCapacity, const void *src, size_t srcSize);
unsigned int ZSTD_isError(size_t code);

size_t
tmxInflateZstd(const void *input, size_t inputSize, void *output, size_t outputSize)
{
    size_t result = ZSTD_decompress(output, outputSize, input, inputSize);
    if (ZSTD_isError(result))
    {
        tmxError(TMX_ERR_FORMAT);
        return 0;
    }
    return result;
}

#endif

size_t
tmxInflate(const char *input, size_t inputSize, TMXgid *output, size_t outputCount, TMX_COMPRESSION compression)
{
    size_t outputSize, base64Size, result = 0;
    void *base64Data;

    outputSize = outputCount * sizeof(TMXgid);
    base64Size = tmxBase64DecodedSize(input, inputSize);
    base64Data = tmxMalloc(base64Size);
    base64Size = tmxBase64Decode(input, inputSize, base64Data, base64Size);

    switch (compression)
    {
        case TMX_COMPRESSION_GZIP: result = tmxInflateGzip(base64Data, base64Size, output, outputSize); break;
        case TMX_COMPRESSION_ZLIB: result = tmxInflateZlib(base64Data, base64Size, output, outputSize); break;
        case TMX_COMPRESSION_ZSTD: result = tmxInflateZstd(base64Data, base64Size, output, outputSize); break;
        case TMX_COMPRESSION_NONE:
            result = TMX_MIN(base64Size, outputSize);
            memcpy(output, base64Data, result);
            break;
        default:
            tmxFree(base64Data);
            tmxError(TMX_ERR_PARAM);
            return 0;
    }

    tmxFree(base64Data);
    result /= sizeof(TMXgid);

    // TMX spec is always little-endian, so swap if host architecture uses big-endianness.
#ifdef TMX_BIG_ENDIAN
    size_t i;
    TMXgid gid;
    for (i = 0; i < result; i++)
    {
        gid       = output[i];
        output[i] = TMX_ENDIAN_SWAP32(gid);
    }
#endif
    return result;
}

size_t
tmxCsvCount(const char *input, size_t inputSize)
{
    size_t i     = 0;
    size_t count = 0;

    while (input[i] && i < inputSize)
    {
        if (input[i++] == ',')
            count++;
    }
    return count;
}

size_t
tmxCsvDecode(const char *input, size_t inputSize, TMXgid *output, size_t outputCount)
{
    if (!input || !inputSize || !outputCount)
        return 0;

    static const char *delim = ", \t\r\n";

    size_t i  = 0;
    char *p   = tmxStringCopy(input, inputSize);

    char *token = strtok(p, delim);
    while (token && i < outputCount)
    {
        output[i++] = tmxParseUint(token);
        token       = strtok(NULL, delim);
    }

    tmxFree(p);
    return i;
}