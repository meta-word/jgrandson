// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jg_skip.h"

static char const * parse_false(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) >= u_over || **u != 'a' ||
        ++(*u) >= u_over || **u != 'l' ||
        ++(*u) >= u_over || **u != 's' ||
        ++(*u) >= u_over || **u != 'e') {
        static char const e[] = "JSON value starting with an \"f\" does not "
            "spell \"false\" as expected.";
        return e;
    }
    v->type = JG_TYPE_FALSE;
    return NULL;
}

static char const * parse_true(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) >= u_over || **u != 'r' ||
        ++(*u) >= u_over || **u != 'u' ||
        ++(*u) >= u_over || **u != 'e') {
        static char const e[] = "JSON value starting with a \"t\" does not "
            "spell \"true\" as expected.";
        return e;
    }
    v->type = JG_TYPE_TRUE;
    return NULL;
}

static char const * parse_null(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) >= u_over || **u != 'u' ||
        ++(*u) >= u_over || **u != 'l' ||
        ++(*u) >= u_over || **u != 'l') {
        static char const e[] = "JSON value starting with an \"n\" does not "
            "spell \"null\" as expected.";
        return e;
    }
    v->type = JG_TYPE_NULL;
    return NULL;
}

static char const * parse_number(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    uint8_t const * str = *u;
    if (**u == '-' && (++(*u) >= u_over || **u < '0' || **u > '9')) {
        static char const e[] = "A minus sign must be followed by a digit.";
        return e;
    }
    if (**u == '0' && ++(*u) < u_over && **u >= '0' && **u <= '9') {
        static char const e[] = "A numeric value starting with a zero may only "
            "consist of multiple characters when the 2nd character is a "
            "decimal point.";
        return e;
    }
    bool has_decimal_point = false;
    do {
        switch (**u) {
        case '\n': case '\t': case '\r': case ' ': case ',': case ']': case '}':
            goto number_parsed;
        case '.':
            if (has_decimal_point) {
                static char const e[] = "Duplicate decimal point?";
                return e;
            }
            has_decimal_point = true;
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            break;
        case 'E': case 'e':
            if (++(*u) >= u_over ||
                ((**u == '+' || **u == '-') && ++(*u) >= u_over) ||
                **u < '0' || **u > '9') {
                static char const e[] = "Numeric exponent parts must include "
                    "at least one digit.";
                return e;
            }
            while (++(*u) < u_over) {
                switch (**u) {
                case '\n': case '\t': case '\r': case ' ': case ',': case ']':
                case '}':
                    goto number_parsed;
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    continue;
                default:
                    {
                        static char const e[] =
                            "Unrecognized character in numeric exponent part.";
                        return e;
                    }
                }
            }
            goto number_parsed;
        default:
            {
                static char const e[] =
                    "Unrecognized character in numeric value.";
                return e;
            }
        }
    } while (++(*u) < u_over);
    number_parsed:
    v->type = JG_TYPE_NUMBER;
    v->str = str;
    v->byte_c = *u - str;
    return NULL;
}

static char const * parse_utf8(
    uint8_t const * * u
) {
    if (**u >= 0xC0 && **u <= 0xDF) { // 2-byte UTF-8 char
        if (*++(*u) >= 0x80 || **u <= 0xBF) {
            return NULL;
        }
    } else if (**u == 0xE0) { // 3-byte UTF-8 char type 1 of 4
        if (*++(*u) >= 0xA0 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF) {
            return NULL;
        }
    } else if (**u >= 0xE1 && **u <= 0xEC) { // 3-byte UTF-8 char type 2 of 4
        if (*++(*u) >= 0x80 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF) {
            return NULL;
        }
    } else if (**u == 0xED) { // 3-byte UTF-8 char type 3 of 4
        if (*++(*u) >= 0x80 && **u <= 0x9F && *++(*u) >= 0x80 && **u <= 0xBF) {
            return NULL;
        }
    } else if (**u == 0xEE || **u == 0xEF) { // 3-byte UTF-8 char type 4 of 4
        if (*++(*u) >= 0x80 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF) {
            return NULL;
        }
    } else if (**u == 0xF0) { // 4-byte UTF-8 char type 1 of 3
        if (*++(*u) >= 0x90 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF &&
            *++(*u) >= 0x80 && **u <= 0xBF) {
            return NULL;
        }
    } else if (**u >= 0xF1 && **u <= 0xF3) { // 4-byte UTF-8 char type 2 of 3
        if (*++(*u) >= 0x80 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF &&
            *++(*u) >= 0x80 && **u <= 0xBF) {
            return NULL;
        }
    } else if (**u == 0xF4) { // 4-byte UTF-8 char type 3 of 3
        if (*++(*u) >= 0x80 && **u <= 0x8F && *++(*u) >= 0x80 && **u <= 0xBF &&
            *++(*u) >= 0x80 && **u <= 0xBF) {
            return NULL;
        }
    }
    static char const e[] = "String contains invalid UTF-8 byte sequence.";
    return e;
}

// parse_element() prototype needed here due to mutual recursion
static char const * parse_element(
    uint8_t const * * u,
    struct jg_val * v
);

static char const * parse_string(
    uint8_t const * * u,
    struct jg_val * v
) {
    uint8_t const * const str = *u + 1;
    for (;;) {
        if (*++(*u) < ' ') {
            static char const e[] = "Control characters (U+0000 through "
                "U+001F) in JSON strings must be escaped (e.g., \"\\n\").";
            return e;
        }
        switch (**u) {
        case '"':
            v->type = JG_TYPE_STRING;
            v->str = str;
            v->byte_c = *u - str;
            return NULL;
        case '\\':
            switch (*++(*u)) {
            case 'b': case 't': case 'n': case 'f': case 'r': case '"':
            case '/': case '\\':
                continue;
            case 'u': // 4 UTF-16 code point encoding hex chars (e.g., "\uE90F")
                if (*++(*u) < '0' || **u > 'f' ||
                    (**u > '9' && **u < 'A') || (**u > 'F' && **u < 'a') ||
                    *++(*u) < '0' || **u > 'f' ||
                    (**u > '9' && **u < 'A') || (**u > 'F' && **u < 'a') ||
                    *++(*u) < '0' || **u > 'f' ||
                    (**u > '9' && **u < 'A') || (**u > 'F' && **u < 'a') ||
                    *++(*u) < '0' || **u > 'f' ||
                    (**u > '9' && **u < 'A') || (**u > 'F' && **u < 'a')) {
                    static char const e[] = "Invalid hexadecimal UTF-16 code "
                        "point sequence following \\u escape sequence.";
                    return e;
                }
                // check surrogate pair range: \uD800 through \uDFFF
                if (((*u)[-3] == 'D' || (*u)[-3] == 'd') && (*u)[-2] > '7') {
                    // check high surrogate pair range: \uDC00 through \uDFFF
                    if ((*u)[-2] > 'B' && (*u)[-2] != 'a' && (*u)[-2] != 'b') {
                        static char const e[] = "UTF-16 high surrogate "
                            "(\\uDC00 through \\uDFFF) not preceded by UTF-16 "
                            "low surrogate (\\uD800 through \\uDBFF).";
                        return e;
                    }
                    // verify that the low surrogate is followed by a high one
                    if (*++(*u) != '/' ||
                        *++(*u) != 'u' ||
                        (*++(*u) != 'D' && **u != 'd') ||
                        *++(*u) < 'C' ||
                            **u > 'f' ||
                            (**u > 'F' && **u < 'c') ||
                        *++(*u) < '0' ||
                            **u > 'f' ||
                            (**u > '9' && **u < 'A') ||
                            (**u > 'F' && **u < 'a') ||
                        *++(*u) < '0' ||
                            **u > 'f' ||
                            (**u > '9' && **u < 'A') ||
                            (**u > 'F' && **u < 'a')) {
                        static char const e[] = "UTF-16 low surrogate "
                            "(\\uD800 through \\uDBFF) not followed by UTF-16 "
                            "high surrogate (\\uDC00 through \\uDFFF).";
                        return e;
                    }
                }
                continue;
            default:
                {
                    static char const e[] = "The backslash escape character "
                        "may only be followed by '\\', '/', ' ', '\"', 'b', "
                        "'f', 'n', 'r', 't', or 'u'.";
                    return e;
                }
            }
        default:
            if (**u > 0x7F) {
                JG_GUARD(parse_utf8(u));
            }
        }
    }
}

static char const * parse_array(
    uint8_t const * * u,
    struct jg_val * v
) {
    uint8_t const * const u_backup = *u;
    do {
        (*u)++;
        skip_element(u);
        v->val_c++;
    } while (**u == ',');
    if (**u != ']') {
        static char const e[] = "Array elements should be followed by a "
            "comma (',') or a closing bracket (']').";
        return e;
    }
    v->type = JG_TYPE_ARRAY;
    if (!v->val_c) {
        return NULL;
    }
    *u = u_backup;
    v->arr = calloc(v->val_c, sizeof(struct jg_val));
    if (!v->arr) {
        static char const e[] =
            "unsuccessful calloc(v->val_c, sizeof(struct jg_val))";
        return e;
    }
    for (struct jg_val * elem = v->arr; elem < v->arr + v->val_c; elem++) {
        (*u)++; // skip '[' or ',' (already checked, so must return true)
        JG_GUARD(parse_element(u, elem));
        (*u)++; // skip last character of the element parsed
        skip_any_whitespace(u, NULL);
    }
    return NULL;
}

static char const * parse_object(
    uint8_t const * * u,
    struct jg_val * v
) {
    uint8_t const * const u_backup = *u;
    do {
        (*u)++;
        skip_any_whitespace(u, NULL);
        if (**u != '"') {
            static char const e[] = "The key of a key-value pair should be a "
                "string.";
            return e;
        }
        skip_string(u, NULL);
        if (**u != ':') {
            static char const e[] = "The key and value of a key-value pair "
                "should be separated by a hyphen (':').";
            return e;
        }
        (*u)++;
        skip_any_whitespace(u, NULL);
        skip_element(u);
        v->keyval_c++;
    } while (**u == ',');
    if (**u != '}') {
        static char const e[] = "Key-value pairs should be followed by a "
            "comma (',') or a closing brace ('}').";
        return e;
    }
    v->type = JG_TYPE_OBJECT;
    if (!v->keyval_c) {
        return NULL;
    }
    *u = u_backup;
    v->obj = calloc(v->keyval_c, sizeof(struct jg_keyval));
    if (!v->obj) {
        static char const e[] =
            "unsuccessful calloc(v->keyval_c, sizeof(struct jg_keyval))";
        return e;
    }
    for (struct jg_keyval * kv = v->obj; kv < v->obj + v->keyval_c; kv++) {
        (*u)++; // skip '{' or ',' (already checked, so must return true)
        JG_GUARD(parse_string(u, &kv->key));
        (*u)++; // skip ':' (already checked, so must return true)
        JG_GUARD(parse_element(u, &kv->val));
        (*u)++; // skip last character of the value element parsed
        skip_any_whitespace(u, NULL);
    }
    return NULL;
}

// Any value parsed by parse_element() has already been skipped over before, so
// all parse_...() calls below can be called safely without u_over checks.
static char const * parse_element(
    uint8_t const * * u,
    struct jg_val * v
) {
    skip_any_whitespace(u, NULL);
    switch (**u) {
    case '"':
        return parse_string(u, v);
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return parse_number(u, NULL, v);
    case '[':
        return parse_array(u, v);
    case 'f':
        return parse_false(u, NULL, v);
    case 'n':
        return parse_null(u, NULL, v);
    case 't':
        return parse_true(u, NULL, v);
    case '{':
        return parse_object(u, v);
    default:
        {
            static char const e[] = "Unrecognized JSON symbol.";
            return e;
        }
    }
}

// According to https://tools.ietf.org/html/rfc8259 (the current JSON spec at
// the time of writing), any type of JSON value (i.e., string, number, array,
// object, true, false, or null) is also valid at the root level.
static char const * parse_root(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    skip_any_whitespace(u, u_over);
    uint8_t const * const u_backup = *u;
    switch (**u) {
    case '"':
        // Make sure the JSON string contains a complete string type...
        JG_GUARD(skip_string(u, u_over));
        *u = u_backup;
        // ...because parse_string() doesn't check u_over.
        return parse_string(u, v);
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
        return parse_number(u, u_over, v);
    case '[':
        // make sure the JSON string contains a complete root array...
        JG_GUARD(skip_array(u, u_over));
        *u = u_backup;
        // ...because parse_array() doesn't check u_over.
        return parse_array(u, v);
    case 'f':
        return parse_false(u, u_over, v);
    case 'n':
        return parse_null(u, u_over, v);
    case 't':
        return parse_true(u, u_over, v);
    case '{':
        // make sure the JSON string contains a complete root object...
        JG_GUARD(skip_object(u, u_over));
        *u = u_backup;
        // ...because parse_array() doesn't check u_over.
        return parse_object(u, v);
    default:
        {
            static char const e[] = "Unrecognized JSON symbol.";
            return e;
        }
    }
}

char const * jg_parse_str(
    char const * json_str,
    size_t byte_size,
    struct jg_val * v
) {
    char const * e = parse_root(
        (uint8_t const * *) &json_str,
        (uint8_t const *) json_str + byte_size,
        v
    );
    if (!e) {
        return NULL;
    }
    // todo: add JSON string context to the error string here
    return e;
}
