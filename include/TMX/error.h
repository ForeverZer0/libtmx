#ifndef TMX_ERROR_H
#define TMX_ERROR_H

#include "typedefs.h"
#include <stdarg.h>

#define TMX_MAX_ERR_MSG 256

#define TMX_ERR_NONE 0
#define TMX_ERR_MEMORY 1
#define TMX_ERR_UNSUPPORTED 2

typedef void (*TMXerrorfunc)(TMXenum errno, const char *message, TMXuserptr user);

void tmxError(TMXenum errno);

void tmxErrorMessage(TMXenum errno, const char *message);

void tmxErrorMessagev(TMXenum errno, const char *format, ...);

const char *tmxErrorString(TMXenum errno);

#endif /* TMX_ERROR_H */