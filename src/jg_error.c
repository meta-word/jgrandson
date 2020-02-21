// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#define _POSIX_C_SOURCE 201112L // newlocale()

#define JG_CONTEXT_BEFORE_MAX_STRLEN 80
#define JG_CONTEXT_AFTER_MAX_STRLEN 80

#include "jgrandson_internal.h"

#include <locale.h> // newlocale()
#include <stdarg.h> // vs(n)printf()

#define JG_REINIT_MSG "To free any existing state and reset Jgrandson to its " \
    "initial state, call jg_reinit()."

// The indices to err_strs match the enum values of jg_ret
static char const * err_strs[] = {
/*00*/ "Success (no error occurred)",
       // wrong state errors
/*01*/ "jg_parse_...() JSON parse functions can only be called when Jgrandson "
       "is in its initial state (i.e., after jg_init() or jg_reinit()). "
       JG_REINIT_MSG,
/*02*/ "jg_[root|arr|obj]_get_...() getter functions can only be called after "
       "a JSON text was parsed with a jg_parse_...() function. " JG_REINIT_MSG,
/*03*/ "jg_[root|arr|obj]_set_...() setter functions can only be called when "
       "Jgrandson is in its initial state (i.e., after jg_init() or "
       "jg_reinit()), or after one or more previous successful setter function "
       "calls. " JG_REINIT_MSG,
/*04*/ "jg_generate_...() JSON generator functions can only be called after at "
       "least one successful jg_[root|arr|obj]_set_...() setter function call "
       "was made. " JG_REINIT_MSG,
       // setter errors
/*05*/ "Jgrandson does not permit setting the root JSON value more than once. "
        JG_REINIT_MSG,
/*06*/ "The 2nd argument to jg_arr_set_...() must be a JSON array type "
       "(i.e., a jg_arr_set_t pointer).",
/*07*/ "The 2nd argument to jg_obj_set_...() must be a JSON object type "
       "(i.e., a jg_obj_set_t pointer).",
/*08*/ "Jgrandson does not permit setting a duplicate key in the same object.",
       // external errors without errno
/*09*/ "Unsuccessful malloc()",
/*10*/ "Unsuccessful calloc()",
/*11*/ "Unsuccessful realloc()",
/*12*/ "Unsuccessful vsprintf()",
/*13*/ "Unsuccessful vsnprintf()",
/*14*/ "Unsuccessful newlocale()",
/*15*/ "Unsuccessful fread()",
/*16*/ "Unsuccessful fwrite()",
       // external errors with errno
/*17*/ "Unsuccessful fopen(): ",
/*18*/ "Unsuccessful fclose(): ",
/*19*/ "Unsuccessful fseeko(..., SEEK_END): ",
/*20*/ "Unsuccessful ftello(): ",
       // parsing errors (with JSON text context)
/*21*/ "Invalid JSON type",
/*22*/ "Unterminated string: closing double-quote ('\"') not found",
/*23*/ "Unterminated array: closing square bracket (']') not found",
/*24*/ "Unterminated object: closing curly brace ('}') not found",
/*25*/ "JSON type starting with an \"f\" does not spell \"false\" as expected",
/*26*/ "JSON type starting with a \"t\" does not spell \"true\" as expected",
/*27*/ "JSON type starting with an \"n\" does not spell \"null\" as expected",
/*28*/ "A minus sign must be followed by a digit",
/*29*/ "A number starting with a zero may only consist of multiple characters "
       "if its 2nd character is a decimal point",
/*30*/ "Number contains an invalid character",
/*31*/ "Numbers must not contain multiple decimal points",
/*32*/ "The exponent part of a number following an \"e\" or \"E\" must contain "
       "one or more digits, the first of which may optionally be preceded by a "
       "a single \"+\" or \"-\" sign only",
/*33*/ "Invalid character in the exponent part of a number",
/*34*/ "Jgrandson does not support parsing numbers with billions of digits",
/*35*/ "String contains invalid UTF-8 byte sequence",
/*36*/ "Control characters (U+0000 through U+001F) in strings must be escaped",
/*37*/ "String contains a backslash escape character not followed by a "
        "'\\', '/', ' ', '\"', 'b', 'f', 'n', 'r', 't', or 'u'",
/*38*/ "String contains an invalid hexadecimal UTF-16 code point sequence "
       "following a \"\\u\" escape sequence",
/*39*/ "String contains an escaped UTF-16 low surrogate (\\uDC00 through "
       "\\uDFFF) not preceded by a UTF-16 high surrogate (\\uD800 through "
       "\\uDBFF)",
/*40*/ "String contains an escaped UTF-16 high surrogate (\\uD800 through "
       "\\uDBFF) not followed by a UTF-16 low surrogate (\\uDC00 through "
       "\\uDFFF).",
/*41*/ "Jgrandson does not support parsing string sizes greater than 4GB",
/*42*/ "Array elements must be followed by a comma (',') or a closing bracket "
       "(']')",
/*43*/ "The key of a key-value pair must be of type string "
       "(i.e., keys must be enclosed in quotation marks)",
/*44*/ "The key and value of a key-value pair must be separated by a colon "
       "(':')",
/*45*/ "Key-value pairs must be followed by a comma (',') or a closing brace "
       "('}')",
/*46*/ "Jgrandson does not allow duplicate keys within the same object",
/*47*/ "A JSON text must contain only one root value (see rfc8259)",
       // getter errors (with JSON text context)
/*48*/ "Expected JSON type \"null\"",
/*49*/ "Expected JSON type \"boolean\"",
/*50*/ "Expected JSON type \"number\"",
/*51*/ "Expected JSON type \"string\"",
/*52*/ "Expected JSON type \"array\"",
/*53*/ "Expected JSON type \"object\"",
/*54*/ "Expected an array long enough to have an element with index %zu "
       "(counting from zero)",
/*55*/ "Expected an array with at least %zu elements",
/*56*/ "Expected an array with at most %zu elements",
/*57*/ "Expected an object with a key named",
/*58*/ "Expected an object with at least %zu key-value pairs",
/*59*/ "Expected an object with at most %zu key-value pairs",
/*60*/ "Expected a string consisting of at least %zu bytes "
       "(excluding null-terminator)",
/*61*/ "Expected a string consisting of no more than %zu bytes "
       "(excluding null-terminator)",
/*62*/ "Expected a string consisting of at least %zu UTF-8 characters",
/*63*/ "Expected a string consisting of no more than %zu UTF-8 characters",
/*64*/ "Expected an integer "
       "(i.e., a number without decimal point or exponent part)",
/*65*/ "Expected an unsigned integer",
/*66*/ "Expected a signed integer no less than %" PRIiMAX,
/*67*/ "Expected a signed integer no greater than %" PRIiMAX,
/*68*/ "Expected an unsigned integer no less than %" PRIuMAX,
/*69*/ "Expected an unsigned integer no greater than %" PRIuMAX,
/*70*/ "Expected a number that can be converted to a floating point type",
/*71*/ "Expected a number within the range representable by type \"float",
/*72*/ "Expected a number within the range representable by type \"double",
/*73*/ "Expected a number within the range representable by type \"long double"
};

static jg_ret get_print_byte_c(
    jg_t * jg,
    int * byte_c,
    char const * fmt,
    va_list args
) {
    return (*byte_c = vsnprintf(NULL, 0, fmt, args)) < 0 ?
        (jg->ret = JG_E_VSNPRINTF) : JG_OK;
}

static jg_ret print_str(
    jg_t * jg,
    char * str,
    char const * fmt,
    va_list args
) {
    return vsprintf(str, fmt, args) < 0 ? (jg->ret = JG_E_VSPRINTF) : JG_OK;
}

jg_ret print_alloc_str(
    jg_t * jg,
    char * * str,
    char const * fmt,
    ...
) {
    int byte_c = 0;
    va_list args;
    va_start(args, fmt);
    JG_GUARD(get_print_byte_c(jg, &byte_c, fmt, args));
    va_end(args);
    *str = malloc(byte_c + 1);
    if (!*str) {
        return jg->ret = JG_E_MALLOC;
    }
    va_start(args, fmt);
    JG_GUARD(print_str(jg, *str, fmt, args));
    va_end(args);
    return JG_OK;
}

static char const * get_errno_str(
    jg_t * jg
) {
#if defined(_WIN32) || defined(_WIN64)
    // TODO: figure out what Window's equivalent to strerror_l() is.
    char* errno_str = strerror(jg->err_val.errn);
#else
    // Retrieve and append the errno's string representation
    locale_t loc = newlocale(LC_ALL, "", (locale_t) 0);
    if (loc == (locale_t) 0) {
        return jg->static_err_str = err_strs[jg->ret = JG_E_NEWLOCALE];
    }
    // strerror_l() is thread-safe (unlike strerror())
    char * errno_str = strerror_l(jg->err_val.errn, loc);
    freelocale(loc);
#endif
    char const* static_err_str = err_strs[jg->ret];
    char * err_str = malloc(strlen(static_err_str) + strlen(errno_str) + 1);
    if (!err_str) {
        return jg->static_err_str = err_strs[jg->ret = JG_E_MALLOC];
    }
    strcpy(err_str, static_err_str);
    strcpy(err_str + strlen(static_err_str), errno_str);
    jg->err_str_needs_free = true;
    return jg->err_str = err_str;
}

static char const * get_contextual_err_str(
    jg_t * jg,
    char const * main_err_str,
    char const * err_mark_before,
    char const * err_mark_after
) {
    size_t line_i = 1;
    size_t char_i = 1;
    char const * err_line = jg->json_text;
    for (char const * c = err_line; c < jg->json_cur;) {
        switch (*c) {
        case '\r':
            if (c + 1 < jg->json_cur && c[1] == '\n') {
                c++;
            }
            // fall through
        case '\n':
            line_i++;
            char_i = 1;
            err_line = ++c;
            continue;
        default:
            char_i += !is_utf8_continuation_byte(*c);
            c++;
        }
    }

    char context_before[JG_CONTEXT_BEFORE_MAX_STRLEN + 1] = {0};
    memcpy(context_before,
        JG_MAX(err_line, jg->json_cur - JG_CONTEXT_BEFORE_MAX_STRLEN),
        JG_MIN(JG_CONTEXT_BEFORE_MAX_STRLEN, jg->json_cur - err_line));

    size_t err_char_size = get_utf8_char_size((const uint8_t *) jg->json_cur,
        (const uint8_t *) jg->json_over);
    char err_char[4 + 1] = {0}; // Accomodate max 4 byte UTF-8 chars
    memcpy(err_char, jg->json_cur, err_char_size);

    char const * newline_after = jg->json_cur + err_char_size;
    while (*newline_after != '\r' && *newline_after != '\n' &&
        ++newline_after < jg->json_over);
    char context_after[JG_CONTEXT_AFTER_MAX_STRLEN + 1] = {0};
    memcpy(context_after, jg->json_cur + err_char_size,
        JG_MIN(JG_CONTEXT_AFTER_MAX_STRLEN, newline_after - jg->json_cur - 1));

    char err_mark_before_default[] = "\033[0;31m"; // ANSI escape code: red
    char err_mark_after_default[] = "\033[0m"; // ANSI escapse code: no color
    if (!err_mark_before) {
        err_mark_before = err_mark_before_default;
    }
    if (!err_mark_after) {
        err_mark_after = err_mark_after_default;
    }
    char * err_str = NULL;
    jg_ret ret = JG_OK;
    if (jg->custom_err_str) {
        ret = print_alloc_str(jg, &err_str,
            "%s: %s: [LINE %zu, CHAR %zu] %s%s%s%s%s",
            main_err_str,
            jg->custom_err_str,
            line_i,
            char_i,
            context_before,
            err_mark_before,
            err_char,
            err_mark_after,
            context_after
        );
        free(jg->custom_err_str);
        jg->custom_err_str = NULL;
    } else {
        ret = print_alloc_str(jg, &err_str,
            "%s: [LINE %zu, CHAR %zu] %s%s%s%s%s",
            main_err_str,
            line_i,
            char_i,
            context_before,
            err_mark_before,
            err_char,
            err_mark_after,
            context_after
        );
    }
    if (ret != JG_OK) {
        return jg->static_err_str = err_strs[ret];
    }
    jg->err_str_needs_free = true;
    return jg->err_str = err_str;
}

char const * jg_get_err_str(
    jg_t * jg,
    char const * err_mark_before,
    char const * err_mark_after
) {
    free_err_str(jg);
    char * main_err_str = NULL;
    jg_ret print_alloc_ret = JG_OK;
    switch (jg->ret) {
    case JG_OK:
    case JG_E_STATE_NOT_PARSE:
    case JG_E_STATE_NOT_GET:
    case JG_E_STATE_NOT_SET:
    case JG_E_STATE_NOT_GENERATE:
    case JG_E_SET_ROOT_ALREADY_SET:
    case JG_E_SET_NOT_ARR:
    case JG_E_SET_NOT_OBJ:
    case JG_E_SET_OBJ_DUPLICATE_KEY:
    case JG_E_MALLOC:
    case JG_E_CALLOC:
    case JG_E_REALLOC:
    case JG_E_VSPRINTF:
    case JG_E_VSNPRINTF:
    case JG_E_NEWLOCALE:
    case JG_E_FREAD:
    case JG_E_FWRITE:
        return jg->static_err_str = err_strs[jg->ret];
    case JG_E_ERRNO_FOPEN:
    case JG_E_ERRNO_FCLOSE:
    case JG_E_ERRNO_FSEEKO:
    case JG_E_ERRNO_FTELLO:
        return get_errno_str(jg);
    case JG_E_GET_ARR_INDEX_OVER:
    case JG_E_GET_ARR_TOO_SHORT:
    case JG_E_GET_ARR_TOO_LONG:
    case JG_E_GET_OBJ_TOO_SHORT:
    case JG_E_GET_OBJ_TOO_LONG:
    case JG_E_GET_STR_BYTE_C_TOO_FEW:
    case JG_E_GET_STR_BYTE_C_TOO_MANY:
    case JG_E_GET_STR_CHAR_C_TOO_FEW:
    case JG_E_GET_STR_CHAR_C_TOO_MANY:
        print_alloc_ret = print_alloc_str(jg, &main_err_str, err_strs[jg->ret],
            jg->err_val.s);
        break;
    case JG_E_GET_NUM_SIGNED_TOO_SMALL:
    case JG_E_GET_NUM_SIGNED_TOO_LARGE:
        print_alloc_ret = print_alloc_str(jg, &main_err_str, err_strs[jg->ret],
            jg->err_val.i);
        break;
    case JG_E_GET_NUM_UNSIGNED_TOO_SMALL:
    case JG_E_GET_NUM_UNSIGNED_TOO_LARGE:
        print_alloc_ret = print_alloc_str(jg, &main_err_str, err_strs[jg->ret],
            jg->err_val.u);
        break;
    default:
        return get_contextual_err_str(jg, err_strs[jg->ret], err_mark_before,
            err_mark_after);
    }
    if (print_alloc_ret != JG_OK) {
        return jg->static_err_str = err_strs[print_alloc_ret];
    }
    char const * err_str = get_contextual_err_str(jg, main_err_str,
        err_mark_before, err_mark_after);
    free(main_err_str);
    return err_str;
}
