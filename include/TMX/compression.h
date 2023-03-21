#ifndef TMX_COMPRESSION_H
#define TMX_COMPRESSION_H

#include <stddef.h>
#include "TMX/typedefs.h"

#define TMX_COMPRESSION_NONE 0
#define TMX_COMPRESSION_GZIP 1
#define TMX_COMPRESSION_ZLIB 2
#define TMX_COMPRESSION_ZSTD 3

#define TMX_ENCODING_NONE   0
#define TMX_ENCODING_CSV    1
#define TMX_ENCODING_BASE64 2

size_t tmxBase64DecodedSize(const char *input, size_t inputLen);

size_t tmxBase64Decode(const char *input, size_t inputLen, void *output, size_t outputLen);

size_t tmxInflateGzip(const void *input, size_t inputLen, void *ouput, size_t ouputLen);

size_t tmxInflateZlib(const void *input, size_t inputLen, void *ouput, size_t ouputLen);

size_t tmxInflateZstd(const void *input, size_t inputLen, void *ouput, size_t ouputLen);

size_t tmxCsvCount(const char *input, size_t inputLen);

size_t tmxCsvDecode(const char *input, size_t inputLen, int *output, size_t outputCount);

size_t tmxInflate(const char *input, size_t inputLen, TMXenum compression, void *output, size_t outputLen);

#endif /* TMX_COMPRESSION_H */