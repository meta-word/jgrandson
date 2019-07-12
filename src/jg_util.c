// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#define _POSIX_C_SOURCE 201112L // newlocale()

#define JG_CONTEXT_BEFORE_MAX_STRLEN 80
#define JG_CONTEXT_AFTER_MAX_STRLEN 80

#include "jgrandson_internal.h"

#include <locale.h> // newlocale()

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
/*04*/ "jg_gen_...() JSON generator functions can only be called after at "
       "least one successful jg_[root|arr|obj]_set_...() setter function call "
       "was made. " JG_REINIT_MSG,
       // external errors without errno
/*05*/ "Unsuccessful malloc()",
/*06*/ "Unsuccessful calloc()",
/*07*/ "Unsuccessful realloc()",
/*08*/ "Unsuccessful newlocale()",
/*09*/ "Unsuccessful fread()",
       // external errors with errno
/*10*/ "Unsuccessful fopen(): ",
/*11*/ "Unsuccessful fclose(): ",
/*12*/ "Unsuccessful fseeko(..., SEEK_END): ",
/*13*/ "Unsuccessful ftello(): ",
       // parsing errors (with JSON text context)
/*14*/ "Invalid JSON type",
/*15*/ "Unterminated string: closing double-quote ('\"') not found",
/*16*/ "Unterminated array: closing square bracket (']') not found",
/*17*/ "Unterminated object: closing curly brace ('}') not found",
/*18*/ "JSON type starting with an \"f\" does not spell \"false\" as expected",
/*19*/ "JSON type starting with a \"t\" does not spell \"true\" as expected",
/*20*/ "JSON type starting with an \"n\" does not spell \"null\" as expected",
/*21*/ "A minus sign must be followed by a digit",
/*22*/ "A number starting with a zero may only consist of multiple characters "
       "if its 2nd character is a decimal point",
/*23*/ "Number contains an invalid character",
/*24*/ "Numbers must not contain multiple decimal points",
/*25*/ "The exponent part of a number following an \"e\" or \"E\" must contain "
       "one or more digits, the first of which may optionally be preceded by a "
       "a single \"+\" or \"-\" sign only",
/*26*/ "Invalid character in the exponent part of a number",
/*27*/ "Jgrandson does not support parsing numbers with billions of digits",
/*28*/ "String contains invalid UTF-8 byte sequence",
/*29*/ "Control characters (U+0000 through U+001F) in strings must be escaped",
/*30*/ "String contains a backslash escape character not followed by a "
        "'\\', '/', ' ', '\"', 'b', 'f', 'n', 'r', 't', or 'u'",
/*31*/ "String contains an invalid hexadecimal UTF-16 code point sequence "
       "following a \"\\u\" escape sequence",
/*32*/ "String contains an escaped UTF-16 high surrogate (\\uDC00 through "
       "\\uDFFF) not preceded by a UTF-16 low surrogate (\\uD800 through "
       "\\uDBFF)",
/*33*/ "String contains an escaped UTF-16 low surrogate (\\uD800 through "
       "\\uDBFF) not followed by a UTF-16 high surrogate (\\uDC00 through "
       "\\uDFFF).",
/*34*/ "Jgrandson does not support parsing string sizes greater than 4GB",
/*35*/ "Array elements must be followed by a comma (',') or a closing bracket "
       "(']')",
/*36*/ "The key of a key-value pair must be of type string "
       "(i.e., keys must be enclosed in quotation marks)",
/*37*/ "The key and value of a key-value pair must be separated by a colon "
       "(':')",
/*38*/ "Key-value pairs must be followed by a comma (',') or a closing brace "
       "('}')",
/*39*/ "Jgrandson does not allow duplicate keys within the same object",
/*40*/ "A JSON text must contain only one root value (see rfc8259)",
       // getter errors
/*41*/ "Expected JSON type \"null\"",
/*42*/ "Expected JSON type \"boolean\"",
/*43*/ "Expected JSON type \"number\"",
/*44*/ "Expected JSON type \"string\"",
/*45*/ "Expected JSON type \"array\"",
/*46*/ "Expected JSON type \"object\"",
/*47*/ "Expected an array long enough to have an element with index %zu "
       "(counting from zero)",
/*48*/ "Expected an array with at least %zu elements",
/*49*/ "Expected an array with at most %zu elements",
/*50*/ "Expected an object with a key named",
/*51*/ "Expected an object with at least %zu key-value pairs",
/*52*/ "Expected an object with at most %zu key-value pairs",
/*53*/ "Expected a string consisting of at least %zu bytes "
       "(excluding null-terminator)",
/*54*/ "Expected a string consisting of no more than %zu bytes "
       "(excluding null-terminator)",
/*55*/ "Expected a string consisting of at least %zu UTF-8 characters",
/*56*/ "Expected a string consisting of no more than %zu UTF-8 characters",
/*57*/ "Expected an integer "
       "(i.e., a number without decimal point or exponent part)",
/*58*/ "Expected an unsigned integer",
/*59*/ "Expected a signed integer no less than %" PRIiMAX,
/*60*/ "Expected a signed integer no greater than %" PRIiMAX,
/*61*/ "Expected an unsigned integer no less than %" PRIuMAX,
/*62*/ "Expected an unsigned integer no greater than %" PRIuMAX,
/*63*/ "Expected a number that can be converted to a floating point type",
/*64*/ "Expected a number within the range representable by type \"float",
/*65*/ "Expected a number within the range representable by type \"double",
/*66*/ "Expected a number within the range representable by type \"long double"
};

jg_t * jg_init(
    void
) {
    return calloc(1, sizeof(struct jgrandson));
}

void free_json_text(
    jg_t * jg
) {
    if (jg->json_text_needs_free) {
        free((char *) jg->json_text);
        jg->json_text = NULL;
        jg->json_text_needs_free = false;
    }
}

void free_err_str(
    jg_t * jg
) {
    if (jg->err_str_needs_free) {
        free((char *) jg->err_str);
        jg->err_str = NULL;
        jg->err_str_needs_free = false;
    }
}

jg_ret set_custom_err_str(
    jg_t * jg,
    char const * custom_err_str
) {
    size_t byte_c = strlen(custom_err_str) + 1;
    if (jg->custom_err_str) {
        jg->custom_err_str = realloc(jg->custom_err_str, byte_c);
        if (!jg->custom_err_str) {
            return jg->ret = JG_E_REALLOC;
        }
    } else {
        jg->custom_err_str = malloc(byte_c);
        if (!jg->custom_err_str) {
            return jg->ret = JG_E_MALLOC;
        }
    }
    memcpy(jg->custom_err_str, custom_err_str, byte_c);
    return JG_OK;
}

// Needed because mutually recursive with free_[array|object]()
static void free_value_in(
    struct jg_val_in * v
);

static void free_array(
    struct jg_arr * arr
) {
    for (struct jg_val_in * elem = arr->elems;
        elem < arr->elems + arr->elem_c; elem++) {
        free_value_in(elem);
    }
    free(arr);
}

static void free_object(
    struct jg_obj * obj
) {
    for (struct jg_pair * pair = obj->pairs;
        pair < obj->pairs + obj->pair_c; pair++) {
        free_value_in(&pair->val);
    }
    free(obj);
}

static void free_value_in(
    struct jg_val_in * v
) {
    switch (v->type) {
    case JG_TYPE_ARR:
        free_array(v->arr);
        return;
    case JG_TYPE_OBJ:
        free_object(v->obj);
        return;
    default:
        return;
    }
}

static void free_all(
    jg_t * jg,
    bool free_jg
) {
    switch (jg->state) {
    case JG_STATE_INIT:
        if (free_jg) {
            free(jg);
        }
        return;
    case JG_STATE_PARSE:
    case JG_STATE_GET:
        free_value_in(&jg->root_in);
        break;
    case JG_STATE_SET:
    case JG_STATE_GENERATE: default:
        ; // todo: free_value_out(&jg->root_out);
    }
    free_json_text(jg);
    free_err_str(jg);
    if (jg->custom_err_str) {
        free(jg->custom_err_str);
    }
    if (free_jg) {
        free(jg);
    } else {
        memset(jg, 0, sizeof(*jg));
    }
}

void jg_free(
    jg_t * jg
) {
    free_all(jg, true);
}

void jg_reinit(
    jg_t * jg
) {
    free_all(jg, false);
}

jg_ret astrcpy(
    jg_t * jg,
	char * * dst,
    char const * src
) {
    size_t byte_c = strlen(src);
    *dst = malloc(byte_c + 1);
    if (*dst) {
        return jg->ret = JG_E_MALLOC;
    }
    memcpy(*dst, src, byte_c);
    (*dst)[byte_c] = '\0';
    return JG_OK;
}

bool is_utf8_continuation_byte(
    char byte
) {
    // True if 0b10XXXXXX, otherwise false.
    return ((uint8_t) byte & 0xC0) == 0x80;
}

static size_t get_utf8_char_size(
    char const * c,
    char const * const c_over
) {
    if (c >= c_over) return 0;
    if (++c == c_over || !is_utf8_continuation_byte(*c)) return 1;
    if (++c == c_over || !is_utf8_continuation_byte(*c)) return 2;
    return ++c == c_over || !is_utf8_continuation_byte(*c) ? 3 : 4;
}

static char const * get_errno_str(
    jg_t * jg
) {
    // Retrieve and append the errno's string representation
    locale_t loc = newlocale(LC_ALL, "", (locale_t) 0);
    if (loc == (locale_t) 0) {
        return jg->err_str = err_strs[jg->ret = JG_E_NEWLOCALE];
    }
    // strerror_l() is thread-safe (unlike strerror())
    char const * static_err_str = err_strs[jg->ret];
    char * errno_str = strerror_l(jg->err_val.errn, loc);
    freelocale(loc);
    char * err_str = malloc(strlen(static_err_str) + strlen(errno_str) + 1);
    if (!err_str) {
        return jg->err_str = err_strs[jg->ret = JG_E_MALLOC];
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

    size_t err_char_size = get_utf8_char_size(jg->json_cur, jg->json_over);
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
#define JG_ASPRINTF(...) \
    int byte_c = snprintf(NULL, 0, __VA_ARGS__); \
    char * err_str = malloc(byte_c + 1); \
    if (!err_str) { \
        return jg->err_str = err_strs[jg->ret = JG_E_MALLOC]; \
    } \
    /* Ignoring the return value considering how unlikely failure is. */ \
    sprintf(err_str, __VA_ARGS__)
    if (jg->custom_err_str) {
        JG_ASPRINTF("%s: %s: [LINE %zu, CHAR %zu] %s%s%s%s%s",
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
        return jg->err_str = err_str;
    }
    JG_ASPRINTF("%s: [LINE %zu, CHAR %zu] %s%s%s%s%s",
        main_err_str,
        line_i,
        char_i,
        context_before,
        err_mark_before,
        err_char,
        err_mark_after,
        context_after
    );
    return jg->err_str = err_str;
#undef JG_ASPRINTF
}

#define JG_GET_ERR_STR_WITH_ERR_VAL(_err_val) \
do { \
    /* Use VLA for str because short-lived with known upper size bound. */ \
    char str[snprintf(NULL, 0, err_strs[jg->ret], _err_val) + 1]; \
    /* Args to s(n)printf() are known constants, so forego on err checking. */ \
    sprintf(str, err_strs[jg->ret], _err_val); \
    return get_contextual_err_str(jg, str, err_mark_before, err_mark_after); \
} while (0)

char const * jg_get_err_str(
    jg_t * jg,
    char const * err_mark_before,
    char const * err_mark_after
) {
    free_err_str(jg);
    switch (jg->ret) {
    case JG_OK:
    case JG_E_STATE_NOT_PARSE:
    case JG_E_STATE_NOT_GET:
    case JG_E_STATE_NOT_SET:
    case JG_E_STATE_NOT_GEN:
    case JG_E_MALLOC:
    case JG_E_CALLOC:
    case JG_E_REALLOC:
    case JG_E_NEWLOCALE:
    case JG_E_FREAD:
        return jg->err_str = err_strs[jg->ret];
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
        JG_GET_ERR_STR_WITH_ERR_VAL(jg->err_val.s); // evilly returns in macro
    case JG_E_GET_NUM_SIGNED_TOO_SMALL:
    case JG_E_GET_NUM_SIGNED_TOO_LARGE:
        JG_GET_ERR_STR_WITH_ERR_VAL(jg->err_val.i); // evilly returns in macro
    case JG_E_GET_NUM_UNSIGNED_TOO_SMALL:
    case JG_E_GET_NUM_UNSIGNED_TOO_LARGE:
        JG_GET_ERR_STR_WITH_ERR_VAL(jg->err_val.u); // evilly returns in macro
    default:
        return get_contextual_err_str(jg, err_strs[jg->ret], err_mark_before,
            err_mark_after);
    }
}
