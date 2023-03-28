#include "internal.h"
#include <stdio.h> // Remove
#include <stdarg.h>

static TMX_ERRNO lastError;
static TMXerrorfunc errorCallback;
static TMXuserptr userPtrValue; 

void
tmxErrorCallback(TMXerrorfunc callback, TMXuserptr user)
{
    errorCallback = callback;
    userPtrValue  = user;
}

void
tmxError(TMX_ERRNO errno)
{
    tmxErrorMessage(errno, tmxErrorString(errno));
}

void
tmxErrorMessage(TMX_ERRNO errno, const char *message)
{
    if (errno == TMX_ERR_NONE)
        return;
    if (lastError == TMX_ERR_NONE)
        lastError = errno;

    if (errorCallback)
        errorCallback(errno, message, userPtrValue);
    else
        fprintf(stderr, "%s\n", message); // TODO: Remove
}

void
tmxErrorFormat(TMX_ERRNO errno, const char *format, ...)
{
    char buffer[TMX_MAX_ERR_MSG];

    va_list argp;
    va_start(argp, format);
    int len = vsnprintf(buffer, TMX_MAX_ERR_MSG - 1, format, argp);
    va_end(argp);

    buffer[len] = '\0';
    tmxErrorMessage(errno, buffer);
}

const char *
tmxErrorString(TMX_ERRNO errno)
{
    switch (errno)
    {
        case TMX_ERR_NONE: return "No error.";
        case TMX_ERR_WARN: return "A warning was emitted.";
        case TMX_ERR_MEMORY: return "A memory allocation failed.";
        case TMX_ERR_UNSUPPORTED: return "An unsupported feature, format, or encoding.";
        case TMX_ERR_FORMAT: return "Unrecognized or unknown format.";
        case TMX_ERR_PARAM: return "An invalid enumeration value was specified.";
        case TMX_ERR_VALUE: return "An invalid or out of range value was specified.";
        case TMX_ERR_INVALID_OPERATION: return "Attempted an operation that is invalid for the current state/context.";
        case TMX_ERR_IO: return "An IO error occurred.";
        case TMX_ERR_PARSE: return "A parsing error occurred.";
        default: return "An unknown error occurred.";
    }
}

TMX_ERRNO
tmxGetError(void)
{
    TMX_ERRNO errno = lastError;
    lastError     = TMX_ERR_NONE;
    return errno;
}