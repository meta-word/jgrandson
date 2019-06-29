// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#define _POSIX_C_SOURCE 201112L // newlocale()

#include "jg_util.h"

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
       // parsing errors (i.e., with JSON string context)
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
/*20*/ "The exponent part of a number must include at least one digit",
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
       "be enclosed in double-quotes)",
/*30*/ "The key and value of a key-value pair must be separated by a hyphen "
       "(':')",
/*31*/ "Key-value pairs must be followed by a comma (',') or a closing brace "
       "('}')"
};

void free_json_str(
    jg_t * jg
) {
    if (jg->json_str_needs_free) {
        free(jg->json_str);
        jg->json_str = NULL;
        jg->json_str_needs_free = false;
    }
}

static void free_err_str(
    jg_t * jg
) {
    if (jg->err_str_needs_free) {
        free(jg->err_str);
        jg->err_str = NULL;
        jg->err_str_needs_free = false;
    }
}

jg_t * jg_init(
    void
) {
    return calloc(1, sizeof(struct jg_opaque));
}

void jg_free(
    jg_t * jg
) {
    (void) jg; // todo
}

char const * jg_get_err_str(
    jg_t * jg
) {
    free_err_str(jg);
    char const * static_err_str = err_strs[jg->ret];
    if (jg->ret <= JG_E_FREAD) {
        jg->err_str = (char *) static_err_str;
        return static_err_str;
    }
    if (jg->ret <= JG_E_ERRNO_FTELLO) {
        // Retrieve and append the errno's string representation
        locale_t loc = newlocale(LC_ALL, "", (locale_t) 0);
        if (loc == (locale_t) 0) {
            static_err_str = err_strs[jg->ret = JG_E_NEWLOCALE];
            jg->err_str = (char *) static_err_str;
            return static_err_str;
        }
        char * errno_str = strerror_l(jg->errnum, loc); // thread-safe
        freelocale(loc);
        char * err_str = malloc(strlen(static_err_str) + strlen(errno_str) + 1);
        if (!err_str) {
            static_err_str = err_strs[jg->ret = JG_E_MALLOC];
            jg->err_str = (char *) static_err_str;
            return static_err_str;
        }
        strcpy(err_str, static_err_str);
        strcpy(err_str + strlen(static_err_str), errno_str);
        jg->err_str_needs_free = true;
        return jg->err_str = err_str;
    }
    return static_err_str;
}
