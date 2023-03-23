#include "tmx/xml.h"
#include "tmx/error.h"
#include "tmx/memory.h"
#include "yxml.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TMXxmlreader
{
    yxml_t scanner;
    yxml_ret_t token;
    char *ptr;
    char *buffer;
    char *memory;
    const char *input;
    const char *str;
};

static inline TMXbool
tmxXmlNextToken(TMXxmlreader *xml)
{
    xml->str++;
    xml->token = yxml_parse(&xml->scanner, *xml->str);
    return *xml->str;
}

static inline void
tmxXmlResetBuffer(TMXxmlreader *xml)
{
    xml->ptr  = xml->buffer;
    *xml->ptr = '\0';
}

const char *
tmxXmlElementName(const TMXxmlreader *xml)
{
    return xml->scanner.elem;
}

TMXbool
tmxXmlMoveToContent(TMXxmlreader *xml)
{
    tmxXmlResetBuffer(xml);

    do
    {
        switch (xml->token)
        {
            case YXML_ELEMEND: return TMX_FALSE;
            case YXML_ELEMSTART:
            case YXML_CONTENT: return TMX_TRUE;
            default: tmxXmlNextToken(xml); break;
        }
    } while (xml->str);

    return TMX_FALSE;
}

TMXbool
tmxXmlReadElement(TMXxmlreader *xml, const char **outName, size_t *outNameSize)
{

    // TODO: Check if already positioned at element start before reading a token?

    while (tmxXmlNextToken(xml))
    {
        if (xml->token == YXML_ELEMEND)
            return TMX_FALSE;

        if (xml->token != YXML_ELEMSTART)
            continue;

        *outName     = xml->scanner.elem;
        *outNameSize = yxml_symlen(&xml->scanner, xml->scanner.elem);
        return TMX_TRUE;
    }
    return TMX_FALSE;

    // do
    // {
    //     if (xml->token == YXML_ELEMEND)
    //         return TMX_FALSE;

    //     if (xml->token != YXML_ELEMSTART)
    //         continue;

    //     *outName = xml->scanner.elem;
    //     *outNameSize = yxml_symlen(&xml->scanner, xml->scanner.elem);
    //     return TMX_TRUE;
    // } while (tmxXmlMoveNext(xml));
    // return TMX_FALSE;
}

TMXbool
tmxXmlReadStringContents(TMXxmlreader *xml, const char **contents, size_t *contentsSize, TMXbool trim)
{
    int c;
    tmxXmlResetBuffer(xml);

    if (xml->token != YXML_CONTENT)
        return TMX_FALSE;

    while (xml->token == YXML_CONTENT)
    {
        for (c = 0; c < sizeof(xml->scanner.data) && xml->scanner.data[c]; ++c)
        {
            *xml->ptr = xml->scanner.data[c];
            xml->ptr++;
        }
        tmxXmlNextToken(xml);
    }

    // Set null-terminator
    *xml->ptr = '\0';

    // If only whitespace, don't consider this valid content.
    if (strspn(xml->buffer, " \n\r\t") == xml->ptr - xml->buffer)
        return TMX_FALSE;

    if (trim)
    {
        // Create copies of the start/end pointers
        char *start = xml->buffer;
        char *end   = xml->ptr;

        while (isspace(*start))
        {
            // Move start pointer forward from the beginning of the string as long as space character is encountered
            start++;
        }

        while (isspace(*(end - 1)))
        {
            // Move end pointer backward from the end of the string as long as space is encountered
            end--;
        }

        // Add a null-terminator at the new end pointer, and calculate sub-length.
        *end          = '\0';
        *contents     = start;
        *contentsSize = (size_t) (end - start);
    }
    else
    {
        *contents     = xml->buffer;
        *contentsSize = (size_t) (xml->ptr - xml->buffer);
    }

    return TMX_TRUE;
}

TMXbool
tmxXmlReadAttr(TMXxmlreader *xml, const char **name, const char **value)
{
    // The only two valid positions the parser should be positioned at.
    if (xml->token == YXML_ELEMSTART || xml->token == YXML_ATTREND)
        tmxXmlNextToken(xml);

    // Reset the write-pointer to the beginning of the buffer.
    xml->ptr = xml->buffer;
    int c;
    while (xml->str)
    {
        switch (xml->token)
        {
            case YXML_OK:
                // Ignore
                break;
            case YXML_ATTRSTART:
                // The beginning of an attribute, assign the name.
                *name = xml->scanner.attr;
                break;
            case YXML_ATTRVAL:
            {
                // Copy the bytes to the buffer, advancing the write buffer as it writes.
                for (c = 0; c < sizeof(xml->scanner.data) && xml->scanner.data[c]; ++c)
                {
                    *xml->ptr = xml->scanner.data[c];
                    xml->ptr++;
                }
                break;
            }
            case YXML_ATTREND:
            {
                // The end of the attribute, append a null-terminator and return success.
                *xml->ptr = '\0';
                *value    = xml->buffer;
                return TMX_TRUE;
            }
            default: return TMX_FALSE;
        }

        // Tokenize the next byte from the document
        tmxXmlNextToken(xml);
    }

    return TMX_FALSE;
}

void
tmxXmlReaderFree(TMXxmlreader *reader)
{
    if (!reader)
        return;

    tmxFree(reader->buffer);
    tmxFree(reader->memory);
    if (reader->input)
        tmxFree((void *) reader->input);

    tmxFree(reader);
}

TMXxmlreader *
tmxXmlReaderInit(const char *input, size_t bufferSize)
{
    TMXxmlreader *reader;
    reader         = tmxCalloc(1, sizeof(TMXxmlreader));
    reader->buffer = tmxMalloc(bufferSize);
    reader->memory = tmxMalloc(bufferSize);
    reader->str    = input;

    yxml_init(&reader->scanner, reader->memory, bufferSize);
    return reader;
}

TMXxmlreader *
tmxXmlReaderInitFromFile(const char *filename, size_t bufferSize)
{
    FILE *fp;
    long len;
    char *buffer;
    TMXxmlreader *xml;

    fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = tmxMalloc(len + 1);
    fread(buffer, 1, len, fp);
    buffer[len] = '\0';

    xml = tmxXmlReaderInit(buffer, bufferSize);
    if (!xml)
    {
        tmxFree(buffer);
        return NULL;
    }

    xml->input = buffer;
    return xml;
}

TMXbool
tmxXmlAssertEOF(TMXxmlreader *xml)
{
    if (yxml_eof(&xml->scanner) == YXML_OK)
        return TMX_TRUE;

    tmxErrorMessage(TMX_ERR_FORMAT, "Expected end of document.");
    return TMX_FALSE;
}