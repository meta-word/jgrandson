// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jg_opaque.h"

// The indices to err_msgs match the enum values of jg_ret
static char const * err_msgs[] = {
/*00*/ "Success (no error occurred)",
       // external library errors
/*01*/ "Unsuccessful fopen()",
/*02*/ "Unsuccessful fclose()",
/*03*/ "Unsuccessful fread()",
/*04*/ "Unsuccessful fseeko(..., SEEK_END)",
/*05*/ "Unsuccessful ftello()",
/*06*/ "Unsuccessful malloc()",
/*07*/ "Unsuccessful calloc()",
       // internal error
/*08*/ "Jgrandson currently doesn't support JSON files/strings larger than 4GB",
       // parsing errors
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

char const * jg_get_err_msg(
    jg_t * jg
) {
    return err_msgs[jg->ret]; // todo: add error context if parse error
}
