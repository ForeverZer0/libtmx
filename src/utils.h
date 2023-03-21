#ifndef TMX_UTILS_H
#define TMX_UTILS_H

#include <stddef.h>

#define TMX_MIN(a, b)          ((a) < (b) ? (a) : (b))
#define TMX_MAX(a, b)          ((a) > (b) ? (a) : (b))
#define TMX_CLAMP(x, min, max) TMX_MAX(min, TMX_MIN(max, x))

#define TMX_UNUSED(x) ((void) x)

char *tmxStringCopy(const char *input, size_t inputLen);

#define tmxStringDup(input) tmxStringCopy(input, 0);

#endif /* TMX_UTILS_H */