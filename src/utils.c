#include "utils.h"
#include <string.h>

#include "TMX/memory.h"

inline char *tmxStringCopy(const char *input, size_t inputLen)
{
    size_t len = inputLen ? inputLen : strlen(input);
    char *result = tmxMalloc(len + 1);
    memcpy(result, input, len);
    result[len] = '\0';
    return result;
}

