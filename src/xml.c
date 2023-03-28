#include "tmx/xml.h"
#include "internal.h"
#include "tmx/memory.h"
#include "yxml.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TMXxmlreader
{
    yxml_t reader;    /** The XML parser state. */
    yxml_ret_t token; /** The current token the parser is positioned at. */
    char *ptr;        /** A pointer that can be moved within the buffer. */
    char *buffer;     /** Scratch buffer for storing parsed values of the current entity. */
    char *memory;     /** Internal buffer used by the XML parser. */
    const char *str;  /** The input string that is being parsed. */
};

static TMX_INLINE TMXbool
tmxXmlNextToken(TMXxmlreader *xml)
{
    xml->str++;
    xml->token = yxml_parse(&xml->reader, *xml->str);
    return *xml->str;
}

static TMX_INLINE void
tmxXmlResetBuffer(TMXxmlreader *xml)
{
    xml->ptr  = xml->buffer;
    *xml->ptr = '\0';
}

const char *
tmxXmlElementName(const TMXxmlreader *xml)
{
    return xml->reader.elem;
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
tmxXmlAssertElement(TMXxmlreader *xml, const char *name)
{
    if (strcmp(xml->reader.elem, name) == 0)
        return TMX_TRUE;

    tmxErrorFormat(TMX_ERR_PARSE, "Expected <%s> element.", name);
    return TMX_FALSE;
}

TMXbool
tmxXmlReadElement(TMXxmlreader *xml, const char **outName, size_t *outNameSize)
{

    while (tmxXmlNextToken(xml))
    {
        if (xml->token == YXML_ELEMEND)
            return TMX_FALSE;

        if (xml->token != YXML_ELEMSTART)
            continue;

        *outName     = xml->reader.elem;
        *outNameSize = yxml_symlen(&xml->reader, xml->reader.elem);
        return TMX_TRUE;
    }
    return TMX_FALSE;

    // do
    // {
    //     if (xml->token == YXML_ELEMEND)
    //         return TMX_FALSE;

    //     if (xml->token != YXML_ELEMSTART)
    //         continue;

    //     *outName = xml->reader.elem;
    //     *outNameSize = yxml_symlen(&xml->reader, xml->reader.elem);
    //     return TMX_TRUE;
    // } while (tmxXmlNextToken(xml));
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
        for (c = 0; c < sizeof(xml->reader.data) && xml->reader.data[c]; ++c)
        {
            *xml->ptr = xml->reader.data[c];
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
                *name = xml->reader.attr;
                break;
            case YXML_ATTRVAL:
            {
                // Copy the bytes to the buffer, advancing the write buffer as it writes.
                for (c = 0; c < sizeof(xml->reader.data) && xml->reader.data[c]; ++c)
                {
                    *xml->ptr = xml->reader.data[c];
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
    tmxFree(reader);
}

TMXxmlreader *
tmxXmlReaderInit(const char *input)
{
    size_t bufferSize = strlen(input); // TODO

    TMXxmlreader *reader;
    reader         = tmxCalloc(1, sizeof(TMXxmlreader));
    reader->buffer = tmxMalloc(bufferSize);
    reader->memory = tmxMalloc(bufferSize);
    reader->str    = input;

    yxml_init(&reader->reader, reader->memory, bufferSize);
    return reader;
}

void
tmxXmlSkipElement(TMXxmlreader *xml)
{
    int level = 0;
    do
    {
        if (xml->token == YXML_ELEMSTART)
            level++;
        else if (xml->token == YXML_ELEMEND)
        {
            level--;
            if (level <= 0)
                break;
        }

    } while (tmxXmlNextToken(xml));

    // while (tmxXmlReadElement(xml, &dummy_name, &dummy_size))
    // {
    // }

    // if (tmxXmlMoveToContent(xml))
    //     tmxXmlSkipElement(xml);
}

void
tmxXmlMoveToElement(TMXxmlreader *xml, const char *name)
{
    do
    {
        if (xml->token == YXML_ELEMSTART)
        {
            if (!name)
                return;

            if (strcmp(name, xml->reader.elem) == 0)
                return;
        }
    } while (tmxXmlNextToken(xml));
}