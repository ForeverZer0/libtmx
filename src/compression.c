#include "TMX/compression.h"
#include <string.h>

size_t tmxBase64DecodedSize(const char *input, size_t inputLen)
{
    return 0; // TODO
}

size_t tmxBase64Decode(const char *input, size_t inputLen, void *output, size_t outputLen)
{
    return 0; // TODO
}

size_t tmxInflateGzip(const void *input, size_t inputLen, void *ouput, size_t ouputLen)
{
    return 0; // TODO
}

size_t tmxInflateZlib(const void *input, size_t inputLen, void *ouput, size_t ouputLen)
{
    return 0; // TODO
}

size_t tmxInflateZstd(const void *input, size_t inputLen, void *ouput, size_t ouputLen)
{
    return 0; // TODO
}

size_t tmxInflate(const char *input, size_t inputLen, TMXenum compression, void *output, size_t outputLen)
{
    

    switch (compression)
    {


    }
    return 0; // TODO
}

size_t tmxCsvCount(const char *input, size_t inputLen)
{
    size_t count = 0;
    for (size_t i = 0; i < inputLen && input[i]; i++)
    {
        if (input[i] == ',')
            count++;
    }
    return count;
}

size_t tmxCsvDecode(const char *input, size_t inputLen, int *output, size_t outputCount)
{   



    return 0; // TODO
}