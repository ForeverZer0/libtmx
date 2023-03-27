#ifndef TMX_ERROR_H
#define TMX_ERROR_H

#include "types.h"
#include <stdarg.h>

/**
 * @brief Maximum length for error message strings, including the null-terminator. 
 * Messages that exceed this hard-limit will be truncated.
 */
#define TMX_MAX_ERR_MSG 256

#define TMX_ERR_NONE              0 /** No error. */
#define TMX_ERR_WARN              1 /** A non-critical error or problem that can be recovered from occurred. */
#define TMX_ERR_MEMORY            2 /** A memory allocation failed. */
#define TMX_ERR_UNSUPPORTED       3 /** Unsupported feature, format, or encoding. */
#define TMX_ERR_FORMAT            4 /** Unrecognized or unknown format. */
#define TMX_ERR_PARAM             5 /** An invalid enumeration value was specified. */
#define TMX_ERR_VALUE             6 /** An invalid or out of range value was specified. */
#define TMX_ERR_INVALID_OPERATION 7 /** Attempted an operation that is invalid for the current state/context. */
#define TMX_ERR_IO                8 /** An IO error occurred. */
#define TMX_ERR_PARSE             9 /** A parsing error occurred. */

/**
 * @brief Prototype for error callbacks.
 *
 * @param errno An error code indicating the general type of error that occurred.
 * @param message A brief description of the error.
 * @param user The user-defined pointer that was specified when setting the callback.
 *
 */
typedef void (*TMXerrorfunc)(TMXenum errno, const char *message, TMXuserptr user);

/**
 * @brief Emits an error of the specified type with a generic error message.
 * @param errno An error code indicating the general type of error that occurred.
 */
void tmxError(TMXenum errno);

/**
 * @brief Emits an error of the specified type and supplies a brief message describing it.
 * @param errno An error code indicating the general type of error that occurred.
 * @param message The message to supply with the error.
 */
void tmxErrorMessage(TMXenum errno, const char *message);

/**
 * @brief Emits an error of the specified type and supplies a brief message describing it.
 * @param errno An error code indicating the general type of error that occurred.
 * @param message A @c sprintf style format message to supply with the error.
 * @param ... Arguments for the format string.
 */
void tmxErrorFormat(TMXenum errno, const char *format, ...);

/**
 * @brief Retrieves a generic error message suitable for the given error type.
 * @param errno An error code indicating the general type of error that occurred.
 * @return The error string. This string must @b not be freed by the caller.
 */
const char *tmxErrorString(TMXenum errno);

/**
 * @brief Retrieves the first error (if any) that occurred since the last call to this function, then
 * resets the error state.
 *
 * @return The stored error state, or @ref TMX_ERR_NONE if no error has occurred.
 */
TMXenum tmxGetError(void);

/**
 * @brief Sets a callback function that will be invoked when errors are emitted by the library.
 *
 * @param callback The function to invoke when an error occurs.
 * @param user A user-defined pointer that will be passed to the callback function.
 */
void tmxErrorCallback(TMXerrorfunc callback, TMXuserptr user);

#endif /* TMX_ERROR_H */