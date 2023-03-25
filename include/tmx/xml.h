#ifndef TMX_XML_H
#define TMX_XML_H

#include "types.h"

/**
 * @brief Opaque type that contains the current XML parsing state.
 *
 * @details The parser is implemented as a fast, non-cached forward only reading "stream access" to XML data. It does not
 * build a DSL that can be traversed, and data must be handled as it is parsed and and emitted from the stream. This can make
 * it somewhat cumbersome to use compared to large feature-rich libraries like libXML2, but offers a very significant boost to speed
 * and memory usage in comparison (by orders of magnitude).
 */
typedef struct TMXxmlreader TMXxmlreader;

/**
 * @brief Initializes a new parser from the XML specified @a text.
 *
 * @param[in] text The XML contents.
 * @param[in] bufferSize The size of the internal buffers used for storing temporary values, element names, etc.
 *
 * @return The initialized parser state.
 */
TMXxmlreader *tmxXmlReaderInit(const char *text, size_t bufferSize);

/**
 * @brief Frees the parser.
 *
 * @param[in] xml The parser state.
 */
void tmxXmlReaderFree(TMXxmlreader *xml);

/**
 * @brief Reads the next element from the current position in the stream.
 *
 * @param[in] xml The parser state.
 * @param[out] name A pointer to receive the name the of element.
 * @param[out] nameSize A pointer to receive the number of bytes located at @a name upon success.
 *
 * @return @ref TMX_TRUE if an element was found, otherwise @ref TMX_FALSE.
 *
 * @note The @a name value is only valid until the parser preforms its next action, and must
 * be copied if it needs to be retained.
 */
TMXbool tmxXmlReadElement(TMXxmlreader *xml, const char **name, size_t *nameSize);

/**
 * @brief Reads the inner string contents of an element from the current position in the stream.
 *
 * @param[in] xml The parser state.
 * @param[out] contents A pointer to receive the contents.
 * @param[out] contentsSize A pointer to receive the number of bytes located at @a contents upon success.
 * @param[in] trim Flag indicating if leading/trailing whitespace should be stripped.
 *
 * @return @ref TMX_TRUE if contents was parsed, otherwise @ref TMX_FALSE.
 *
 * @note The @a contents value is only valid until the parser preforms its next action, and must
 * be copied if it needs to be retained.
 */
TMXbool tmxXmlReadStringContents(TMXxmlreader *xml, const char **contents, size_t *contentsSize, TMXbool trim);

/**
 * @brief Attempts to read an attribute from the current position in the stream.
 *
 * @param[in] xml The parser state.
 * @param[out] name A pointer to receive the name the of attribute.
 * @param[out] value A pointer to receive the value of the attribute.
 *
 * @return @ref TMX_TRUE if an attribute was parsed, otherwise @ref TMX_FALSE.
 *
 * @note The @a name and @a value are only valid until the parser preforms its next action, and must
 * be copied if they need to be retained.
 */
TMXbool tmxXmlReadAttr(TMXxmlreader *xml, const char **name, const char **value);

/**
 * @brief Moves the cursor position to the beginning of the content section of an element, which may be either
 * string contents or child elements.
 *
 * @param[in] xml The parser state.
 * @return @ref TMX_TRUE if current element has inner contents, otherwise @ref TMX_FALSE.
 */
TMXbool tmxXmlMoveToContent(TMXxmlreader *xml);

/**
 * @brief Retrieves the name of the current element.
 * 
 * @param[in] xml The parser state to query.
 * @return The name of the current element.
*/
const char *tmxXmlElementName(const TMXxmlreader *xml);

/**
 * @brief Tests whether the expected end of the document is indeed the end of the document.
 * @param[in] xml The parser state to query.
 * @return @ref TMX_TRUE if scanning results are as expected, otherwise @ref TMX_FALSE.
*/
TMXbool tmxXmlAssertEOF(TMXxmlreader *xml);

/**
 * @brief Skips the current element and all of its children.
 * @param[in] xml The parser state. 
 */
void tmxXmlSkipElement(TMXxmlreader *xml);

/**
 * @brief Moves the parser to the beginning of an element.
 * 
 * @param[in] xml The parser state.
 * @param[in] name A name of element to match, or @c NULL to the first element encountered.
 */
void tmxXmlMoveToElement(TMXxmlreader *xml, const char *name);

/**
 * @brief Asserts that the current element has the specified name, emitting a parsing error otherwise.
 * 
 * @param[in] xml The parser state.
 * @param[in] name The element name to test.
 * @return @ref TMX_TRUE if element name matches, otherwise @ref TMX_FALSE.
 */
TMXbool tmxXmlAssertElement(TMXxmlreader *xml, const char *name);

#endif /* TMX_XML_H */