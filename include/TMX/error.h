#ifndef TMX_ERROR_H
#define TMX_ERROR_H

#include "typedefs.h"
#include <stdarg.h>

/**
 * @brief Maximum length for error message strings, including the null-terminator.
 */
#define TMX_MAX_ERR_MSG 256

#define TMX_ERR_NONE              0 /** No error. */
#define TMX_ERR_MEMORY            1 /** A memory allocation failed. */
#define TMX_ERR_UNSUPPORTED       2 /** Unsupported feature, format, or encoding. */
#define TMX_ERR_FORMAT            3 /** Unrecognized or unknown format. */
#define TMX_ERR_PARAM             4 /** An invalid enumeration value was specified. */
#define TMX_ERR_VALUE             5 /** An invalid or out of range value was specified. */
#define TMX_ERR_INVALID_OPERATION 6 /** Attempted an operation that is invalid for the current state/context. */

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
 * @brief Emits an error of the specified and supplies a brief message describing it.
 * @param errno An error code indicating the general type of error that occurred.
 * @param message The message to supply with the error.
 */
void tmxErrorMessage(TMXenum errno, const char *message);

/**
 * @brief Emits an error of the specified and supplies a brief message describing it.
 * @param errno An error code indicating the general type of error that occurred.
 * @param message A format message to supply with the error.
 * @param ... Arguments for the format string, same as @c printf arguments.
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
 * @param callback The function to call when an error occurs.
 * @param user A user-defined pointer that will be passed to the callback function.
 */
void tmxSetErrorCallback(TMXerrorfunc callback, TMXuserptr user);

#endif /* TMX_ERROR_H */