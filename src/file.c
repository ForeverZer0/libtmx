#include "cwalk.h"
#include "tmx/file.h"
#include "tmx/memory.h"
#include <stdio.h>
#include <string.h>

static TMXreadfunc fileRead;
static TMXfreefunc fileFree;
static TMXuserptr fileUserPtr;

size_t
tmxFileAbsolutePath(const char *path, const char *basePath, char *buffer, size_t bufferSize)
{
    size_t dirLen;
    ;
    cwk_path_get_dirname(basePath, &dirLen);

    char dirBuffer[dirLen + 1];
    memcpy(dirBuffer, basePath, dirLen);
    dirBuffer[dirLen] = '\0';

    return cwk_path_get_absolute(dirBuffer, path, buffer, bufferSize);
}

size_t
tmxFileDirectory(const char *path)
{
    const char *fs = strrchr(path, '/');
    const char *bs = strrchr(path, '\\');

    const char *last = fs > bs ? fs : bs;
    if (!last)
        return 0;

    return (size_t) ((last + 1) - path);
}

static char *
tmxFileReadImpl(const char *path, const char *basePath)
{
    char *result = NULL;
    size_t len;

    FILE *fp = NULL;
    fp       = fopen(path, "r");

    if (!fp && basePath)
    {
        char buffer[TMX_MAX_PATH];
        len = tmxFileAbsolutePath(path, basePath, buffer, TMX_MAX_PATH);
        if (!len)
            return NULL;

        fp = fopen(buffer, "r");
    }

    if (!fp)
        return NULL;

    fseek(fp, 0, SEEK_END);
    len = (size_t) ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (len)
    {
        result = tmxMalloc(len + 1);
        if (fread(result, 1, len, fp) != len)
        {
            tmxFree(result);
            result = NULL;
        }
        else
        {
            result[len] = '\0';
        }
    }

    fclose(fp);
    return result;
}

char *
tmxFileRead(const char *path, const char *basePath)
{
    if (!path)
        return NULL;

    if (fileRead)
    {
        char *result = NULL;
        size_t len;

        const char *userBuffer = fileRead(path, basePath, fileUserPtr);
        if (userBuffer)
        {
            len    = strlen(userBuffer);
            result = tmxMalloc(len + 1);
            memcpy(result, userBuffer, len);
            result[len] = '\0';
            if (fileFree)
                fileFree((void *) userBuffer, fileUserPtr);
            return result;
        }
    }

    return tmxFileReadImpl(path, basePath);
}