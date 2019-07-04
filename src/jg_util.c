// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#define _POSIX_C_SOURCE 201112L // newlocale()

#define JG_CONTEXT_BEFORE_MAX_STRLEN 80
#define JG_CONTEXT_AFTER_MAX_STRLEN 80

#include "jgrandson_internal.h"

#include <locale.h> // newlocale()

// The indices to err_strs match the enum values of jg_ret
static char const * err_strs[] = {
/*00*/ "Success (no error occurred)",
       // external errors without errno
/*01*/ "Unsuccessful malloc()",
/*02*/ "Unsuccessful calloc()",
/*03*/ "Unsuccessful newlocale()",
/*04*/ "Unsuccessful fread()",
       // external errors with errno
/*05*/ "Unsuccessful fopen(): ",
/*06*/ "Unsuccessful fclose(): ",
/*07*/ "Unsuccessful fseeko(..., SEEK_END): ",
/*08*/ "Unsuccessful ftello(): ",
       // parsing errors (i.e., with JSON text context)
/*09*/ "Invalid JSON type",
/*10*/ "Unterminated string: closing double-quote ('\"') not found",
/*11*/ "Unterminated array: closing square bracket (']') not found",
/*12*/ "Unterminated object: closing curly brace ('}') not found",
/*13*/ "JSON type starting with an \"f\" does not spell \"false\" as expected",
/*14*/ "JSON type starting with a \"t\" does not spell \"true\" as expected",
/*15*/ "JSON type starting with an \"n\" does not spell \"null\" as expected",
/*16*/ "A minus sign must be followed by a digit",
/*17*/ "A number starting with a zero may only consist of multiple characters "
       "if its 2nd character is a decimal point",
/*18*/ "Number contains an invalid character",
/*19*/ "Numbers must not contain multiple decimal points",
/*20*/ "The exponent part of a number following an \"e\" or \"E\" must contain "
       "one or more digits, the first of which may optionally be preceded by a "
       "a single \"+\" or \"-\" sign only",
/*21*/ "Invalid character in the exponent part of a number",
/*22*/ "String contains invalid UTF-8 byte sequence",
/*23*/ "Control characters (U+0000 through U+001F) in strings must be escaped",
/*24*/ "String contains a backslash escape character not followed by a "
        "'\\', '/', ' ', '\"', 'b', 'f', 'n', 'r', 't', or 'u'",
/*25*/ "String contains an invalid hexadecimal UTF-16 code point sequence "
       "following a \"\\u\" escape sequence",
/*26*/ "String contains an escaped UTF-16 high surrogate (\\uDC00 through "
       "\\uDFFF) not preceded by a UTF-16 low surrogate (\\uD800 through "
       "\\uDBFF)",
/*27*/ "String contains an escaped UTF-16 low surrogate (\\uD800 through "
       "\\uDBFF) not followed by a UTF-16 high surrogate (\\uDC00 through "
       "\\uDFFF).",
/*28*/ "Array elements must be followed by a comma (',') or a closing bracket "
       "(']')",
/*29*/ "The key of a key-value pair must be of type string (i.e., keys must "
       "be enclosed in quotation marks)",
/*30*/ "The key and value of a key-value pair must be separated by a colon "
       "(':')",
/*31*/ "Key-value pairs must be followed by a comma (',') or a closing brace "
       "('}')",
/*32*/ "A JSON text must contain only one root value (see rfc8259)",
/*34*/ "", // todo
/*35*/ "",
/*36*/ "",
/*37*/ "",
/*38*/ "",
/*39*/ "",
/*40*/ "",
/*41*/ "",
/*42*/ "",
/*43*/ "",
/*44*/ ""
};

jg_t * jg_init(
    void
) {
    return calloc(1, sizeof(struct jgrandson));
}

void free_json_str(
    jg_t * jg
) {
    if (jg->json_str_needs_free) {
        free((uint8_t *) jg->json_str);
        jg->json_str = NULL;
        jg->json_str_needs_free = false;
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

// Needed because free_array() and free_object() are mutually recursive
static void free_array(
    struct jg_val * v
);

static void free_object(
    struct jg_val * v
) {
    for (struct jg_keyval * kv = v->obj; kv < v->obj + v->keyval_c; kv++) {
        switch (kv->val.type) {
        case JG_TYPE_OBJ:
            free_object(&kv->val);
            continue;
        case JG_TYPE_ARR:
            free_array(&kv->val);
            continue;
        default:
            continue;
        }
    }
    free(v->obj);
}

static void free_array(
    struct jg_val * v
) {
    for (struct jg_val * elem = v->arr; elem < v->arr + v->elem_c; elem++) {
        switch (elem->type) {
        case JG_TYPE_OBJ:
            free_object(elem);
            continue;
        case JG_TYPE_ARR:
            free_array(elem);
            continue;
        default:
            continue;
        }
    }
    free(v->arr);
}

void jg_free(
    jg_t * jg
) {
    switch (jg->root_val.type) {
    case JG_TYPE_OBJ:
        free_object(&jg->root_val);
        break;
    case JG_TYPE_ARR:
        free_array(&jg->root_val);
        break;
    default:
        break;
    }
    free_json_str(jg);
    free_err_str(jg);
    free(jg);
}

jg_ret check_state(
    jg_t * jg,
    enum jg_state state
) {
    if (jg->state != state) {
        jg->err_expected.state = state;
        jg->err_received.state = jg->state;
        return jg->ret = JG_E_WRONG_STATE;
    }
    return JG_OK;
}

static size_t get_utf8_char_size(
    uint8_t const * u,
    uint8_t const * const u_over
) {
    if (u >= u_over) return 0;
    if (++u == u_over || JG_IS_1ST_UTF8_BYTE(*u)) return 1;
    if (++u == u_over || JG_IS_1ST_UTF8_BYTE(*u)) return 2;
    return ++u == u_over || JG_IS_1ST_UTF8_BYTE(*u) ? 3 : 4;
}

char const * jg_get_err_str(
    jg_t * jg,
    char const * parse_err_mark_before,
    char const * parse_err_mark_after
) {
    if (jg->ret != JG_E_GET_STR_CUSTOM) {
        free_err_str(jg);
    }
    char const * static_err_str = err_strs[jg->ret];
    if (jg->ret <= JG_E_FREAD) {
        return jg->err_str = static_err_str;
    }
    if (jg->ret <= JG_E_ERRNO_FTELLO) {
        // Retrieve and append the errno's string representation
        locale_t loc = newlocale(LC_ALL, "", (locale_t) 0);
        if (loc == (locale_t) 0) {
            return jg->err_str = err_strs[jg->ret = JG_E_NEWLOCALE];
        }
        // strerror_l() is thread-safe (unlike strerror())
        char * errno_str = strerror_l(jg->err_received.errn, loc);
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
    // todo: differentiate between JG_E_PARSE and JG_E_GET error string handling
    size_t line_i = 1;
    size_t char_i = 1;
    uint8_t const * err_line = jg->json_str;
    for (uint8_t const * u = err_line; u < jg->json_cur;) {
        switch (*u) {
        case '\r':
            if (u + 1 < jg->json_cur && u[1] == '\n') {
                u++;
            }
            // fall through
        case '\n':
            line_i++;
            char_i = 1;
            err_line = ++u;
            continue;
        default:
            char_i += JG_IS_1ST_UTF8_BYTE(*u++); // How dare you incr in a macro
        }
    }

    char context_before[JG_CONTEXT_BEFORE_MAX_STRLEN + 1] = {0};
    memcpy(context_before,
        JG_MAX(err_line, jg->json_cur - JG_CONTEXT_BEFORE_MAX_STRLEN),
        JG_MIN(JG_CONTEXT_BEFORE_MAX_STRLEN, jg->json_cur - err_line));

    size_t err_char_size = get_utf8_char_size(jg->json_cur, jg->json_over);
    char err_char[4 + 1] = {0}; // Accomodate max 4 byte UTF-8 chars
    memcpy(err_char, jg->json_cur, err_char_size);

    uint8_t const * newline_after = jg->json_cur + err_char_size;
    while (*newline_after != '\r' && *newline_after != '\n' &&
        ++newline_after < jg->json_over);
    char context_after[JG_CONTEXT_AFTER_MAX_STRLEN + 1] = {0};
    memcpy(context_after, jg->json_cur + err_char_size,
        JG_MIN(JG_CONTEXT_AFTER_MAX_STRLEN, newline_after - jg->json_cur - 1));

    char parse_err_mark_before_default[] = "\033[0;31m"; // ANSI esc code: red
    char parse_err_mark_after_default[] = "\033[0m"; // ANSI esc code: no color
    if (!parse_err_mark_before) {
        parse_err_mark_before = parse_err_mark_before_default;
    }
    if (!parse_err_mark_after) {
        parse_err_mark_after = parse_err_mark_after_default;
    }

#define JG_ASPRINTF(...) \
    int byte_c = snprintf(NULL, 0, __VA_ARGS__); \
    char * err_str = malloc(byte_c + 1); \
    if (!err_str) { \
        return jg->err_str = err_strs[jg->ret = JG_E_MALLOC]; \
    } \
    /* Ignoring the return value this time given how unlikely failure is. */ \
    sprintf(err_str, __VA_ARGS__)

    JG_ASPRINTF("%s: [LINE %zu, CHAR %zu] %s%s%s%s%s",
        static_err_str,
        line_i,
        char_i,
        context_before,
        parse_err_mark_before,
        err_char,
        parse_err_mark_after,
        context_after
    );

#undef JG_ASPRINTF

    return jg->err_str = err_str;
}
