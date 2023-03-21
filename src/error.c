#include "TMX/error.h"
#include <stddef.h>
#include <stdio.h>


static TMXerrorfunc errorCallback;
static TMXuserptr userPtr;

TMXerrorfunc tmxSetErrorCallback(TMXerrorfunc callback, TMXuserptr user)
{
    TMXerrorfunc previous = errorCallback;
    errorCallback = callback;
    userPtr = user;
    return previous;
}

void tmxError(TMXenum errno)
{
    tmxErrorMessage(errno, tmxErrorString(errno));
}

void tmxErrorMessage(TMXenum errno, const char *message)
{
    if (errno !=)
    if (errorCallback)
        errorCallback(errno, message, userPtr);
    else
        fprintf(stderr, "%s\n", message);
}

void tmxErrorMessagev(TMXenum errno, const char *format, ...)
{
    char buffer[TMX_MAX_ERR_MSG];

    va_list argp;
    va_start(argp, format);
    int len = vsnprintf(buffer, TMX_MAX_ERR_MSG - 1, format, argp);
    va_end(argp);

    buffer[len] = '\0';
    tmxErrorMessage(errno, buffer);
}

const char *tmxErrorString(TMXenum errno)
{
    switch (errno)
    {
        case TMX_ERR_NONE: return "No error.";
        case TMX_ERR_MEMORY: return "A memory allocation failed.";
        case TMX_ERR_UNSUPPORTED: return "Feature is not supported.";
        default: return "An unknown error occurred.";
    }
}