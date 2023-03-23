#include "tmx/error.h"
#include <stddef.h>
#include <stdio.h>

static TMXenum lastError;
static TMXerrorfunc errorCallback;
static TMXuserptr userPtrValue;

void
tmxSetErrorCallback(TMXerrorfunc callback, TMXuserptr user)
{
    errorCallback = callback;
    userPtrValue  = user;
}

void
tmxError(TMXenum errno)
{
    tmxErrorMessage(errno, tmxErrorString(errno));
}

void
tmxErrorMessage(TMXenum errno, const char *message)
{
    if (errno == TMX_ERR_NONE)
        return;
    if (lastError == TMX_ERR_NONE)
        lastError = errno;

    if (errorCallback)
        errorCallback(errno, message, userPtrValue);
    else
        fprintf(stderr, "%s\n", message);
}

void
tmxErrorFormat(TMXenum errno, const char *format, ...)
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
tmxErrorString(TMXenum errno)
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

TMXenum
tmxGetError(void)
{
    TMXenum errno = lastError;
    lastError     = TMX_ERR_NONE;
    return errno;
}