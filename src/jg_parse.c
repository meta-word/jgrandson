// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#define _POSIX_C_SOURCE 201112L // ftello() and fseeko()'s off_t
#define _FILE_OFFSET_BITS 64 // Ensure off_t is 64 bits (possibly redundant)

#include "jgrandson_internal.h"

static void skip_any_whitespace(
    char const * * c,
    char const * const c_over
) {
    while (*c < c_over &&
        (**c == ' ' || **c == '\n' || **c == '\t' || **c == '\r')) {
        (*c)++;
    }
}

// For cases where it's already known that the JSON string buffer is long enough
static void reskip_any_whitespace(
    char const * * c
) {
    while (**c == ' ' || **c == '\n' || **c == '\t' || **c == '\r') {
        (*c)++;
    }
}

// Try to go to set *c to the char after the string's closing quotation mark,
// but return an error if c_over was reached before that mark was found.
static jg_ret skip_string(
    char const * * c,
    char const * const c_over
) {
    // *c assumed to point to the opening quotation mark.
    char const * const c_backup = *c;
    while (++(*c) < c_over) {
        if (**c == '"') {
            (*c)++;
            return JG_OK;
        }
        if (**c == '\\') {
            if (++(*c) >= c_over) {
                break;
            }
        }
    }
    *c = c_backup; // Set c to the opening " to provide it as error context.
    return JG_E_PARSE_UNTERM_STR;
}

// For strings that have previously gone through skip_string() successfully
static void reskip_string(
    char const * * c
) {
    // *c assumed to point to the opening quotation mark.
    while (*++(*c) != '"') {
        if (**c == '\\') {
            (*c)++;
        }
    }
    (*c)++;
}

// Try to go to set *c to the char after the array's closing bracket (']'),
// but return an error if c_over was reached before that bracket was found.
static jg_ret skip_array(
    char const * * c,
    char const * const c_over
) {
    // *c assumed to point to the opening bracket ('[').
    char const * const c_backup = (*c)++;
    while (*c < c_over) {
        switch (**c) {
        case '"':
            JG_GUARD(skip_string(c, c_over));
            continue;
        case '[':
            JG_GUARD(skip_array(c, c_over));
            continue;
        case ']':
            (*c)++;
            return JG_OK;
        default:
            (*c)++;
            continue;
        }
    }
    *c = c_backup; // Set c to the opening [ to provide it as error context.
    return JG_E_PARSE_UNTERM_ARR;
}

// For arrays that have previously been successfully skipped
static void reskip_array(
    char const * * c
) {
    // *c assumed to point to the opening bracket ('[').
    (*c)++;
    while (**c != ']') {
        switch (**c) {
        case '"':
            reskip_string(c);
            continue;
        case '[':
            reskip_array(c);
            continue;
        default:
            (*c)++;
            continue;
        }
    }
    (*c)++;
}

// Try to go to set *c to the char after the object's closing brace ('}'),
// but return an error if c_over was reached before that brace was found.
static jg_ret skip_object(
    char const * * c,
    char const * const c_over
) {
    // *c assumed to point to the opening brace ('{').
    char const * const c_backup = (*c)++;
    while (*c < c_over) {
        switch (**c) {
        case '"':
            JG_GUARD(skip_string(c, c_over));
            continue;
        case '{':
            JG_GUARD(skip_object(c, c_over));
            continue;
        case '}':
            (*c)++;
            return JG_OK;
        default:
            (*c)++;
            continue;
        }
    }
    *c = c_backup; // Set c to the opening { to provide it as error context.
    return JG_E_PARSE_UNTERM_OBJ;
}

// For objects that have previously been successfully skipped
static void reskip_object(
    char const * * c
) {
    // *c assumed to point to the opening brace ('{').
    (*c)++;
    while (**c != '}') {
        switch (**c) {
        case '"':
            reskip_string(c);
            continue;
        case '{':
            reskip_object(c);
            continue;
        default:
            (*c)++;
            continue;
        }
    }
    (*c)++;
}

// For elements that have previously been successfully skipped
static void reskip_element(
    char const * * c
) {
    reskip_any_whitespace(c);
    switch (**c) {
    case '"':
        reskip_string(c);
        return;
    case '[':
        reskip_array(c);
        return;
    case '{':
        reskip_object(c);
        return;
    default:
        while (**c != ',' && **c != ']' && **c != '}') {
            (*c)++; // reskip number/true/false/null/whitespace
        }
    }
}

static jg_ret parse_null(
    char const * * c,
    char const * const c_over,
    struct jg_val_in * v
) {
    v->json = *c;
    if (++(*c) == c_over || **c != 'u' ||
        ++(*c) == c_over || **c != 'l' ||
        ++(*c) == c_over || **c != 'l') {
        return JG_E_PARSE_NULL;
    }
    (*c)++;
    v->type = JG_TYPE_NULL;
    return JG_OK;
}

static jg_ret parse_false(
    char const * * c,
    char const * const c_over,
    struct jg_val_in * v
) {
    v->json = *c;
    if (++(*c) == c_over || **c != 'a' ||
        ++(*c) == c_over || **c != 'l' ||
        ++(*c) == c_over || **c != 's' ||
        ++(*c) == c_over || **c != 'e') {
        return JG_E_PARSE_FALSE;
    }
    (*c)++;
    v->type = JG_TYPE_BOOL;
    return JG_OK;
}

static jg_ret parse_true(
    char const * * c,
    char const * const c_over,
    struct jg_val_in * v
) {
    v->json = *c;
    if (++(*c) == c_over || **c != 'r' ||
        ++(*c) == c_over || **c != 'u' ||
        ++(*c) == c_over || **c != 'e') {
        return JG_E_PARSE_TRUE;
    }
    (*c)++;
    v->type = JG_TYPE_BOOL;
    v->bool_is_true = true;
    return JG_OK;
}

static jg_ret parse_number(
    char const * * c,
    char const * const c_over,
    struct jg_val_in * v
) {
    v->json = *c;
    if (**c == '-' && (++(*c) == c_over || **c < '0' || **c > '9')) {
        return JG_E_PARSE_NUM_SIGN;
    }
    if (**c == '0' && ++(*c) != c_over && **c >= '0' && **c <= '9') {
        return JG_E_PARSE_NUM_LEAD_ZERO;
    }
    bool has_decimal_point = false;
    do {
        switch (**c) {
        case '\n': case '\t': case '\r': case ' ': case ',': case ']': case '}':
            goto number_parsed;
        case '.':
            if (has_decimal_point) {
                return JG_E_PARSE_NUM_MULTIPLE_POINTS;
            }
            has_decimal_point = true;
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            break;
        case 'E': case 'e':
            if (++(*c) == c_over ||
                ((**c == '+' || **c == '-') && ++(*c) == c_over) ||
                **c < '0' || **c > '9') {
                return JG_E_PARSE_NUM_EXP_HEAD_INVALID;
            }
            while (++(*c) != c_over) {
                switch (**c) {
                case '\n': case '\t': case '\r': case ' ': case ',': case ']':
                case '}':
                    goto number_parsed;
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    continue;
                default:
                    return JG_E_PARSE_NUM_EXP_INVALID;
                }
            }
            goto number_parsed;
        default:
            return JG_E_PARSE_NUM_INVALID;
        }
    } while (++(*c) != c_over);
    number_parsed:
    v->type = JG_TYPE_NUM;
    size_t byte_c = *c - v->json;
    if (byte_c > UINT32_MAX) {
        return JG_E_PARSE_NUM_TOO_LARGE; // 4 billion digits is a bit excessive
    }
    v->byte_c = byte_c;
    return JG_OK;
}

static jg_ret parse_utf8(
    uint8_t const * * u
) {
    uint8_t const * const u_backup = *u;
    if (**u >= 0xC0 && **u <= 0xDF) { // 2-byte UTF-8 char
        if (*++(*u) >= 0x80 || **u <= 0xBF) {
            return JG_OK;
        }
    } else if (**u == 0xE0) { // 3-byte UTF-8 char type 1 of 4
        if (*++(*u) >= 0xA0 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF) {
            return JG_OK;
        }
    } else if (**u >= 0xE1 && **u <= 0xEC) { // 3-byte UTF-8 char type 2 of 4
        if (*++(*u) >= 0x80 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF) {
            return JG_OK;
        }
    } else if (**u == 0xED) { // 3-byte UTF-8 char type 3 of 4
        if (*++(*u) >= 0x80 && **u <= 0x9F && *++(*u) >= 0x80 && **u <= 0xBF) {
            return JG_OK;
        }
    } else if (**u == 0xEE || **u == 0xEF) { // 3-byte UTF-8 char type 4 of 4
        if (*++(*u) >= 0x80 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF) {
            return JG_OK;
        }
    } else if (**u == 0xF0) { // 4-byte UTF-8 char type 1 of 3
        if (*++(*u) >= 0x90 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF &&
            *++(*u) >= 0x80 && **u <= 0xBF) {
            return JG_OK;
        }
    } else if (**u >= 0xF1 && **u <= 0xF3) { // 4-byte UTF-8 char type 2 of 3
        if (*++(*u) >= 0x80 && **u <= 0xBF && *++(*u) >= 0x80 && **u <= 0xBF &&
            *++(*u) >= 0x80 && **u <= 0xBF) {
            return JG_OK;
        }
    } else if (**u == 0xF4) { // 4-byte UTF-8 char type 3 of 3
        if (*++(*u) >= 0x80 && **u <= 0x8F && *++(*u) >= 0x80 && **u <= 0xBF &&
            *++(*u) >= 0x80 && **u <= 0xBF) {
            return JG_OK;
        }
    }
    *u = u_backup; // Point to 1st byte as expected when printing error context
    return JG_E_PARSE_STR_UTF8_INVALID;
}

static jg_ret parse_string(
    char const * * c,
    struct jg_val_in * v
) {
    // *c assumed to point to the opening quotation mark.
    v->json = ++(*c);
    for (;; (*c)++) {
        if (**c < 0) {
            JG_GUARD(parse_utf8((uint8_t const * *) c));
            continue;
        }
        if (**c < ' ') {
            return JG_E_PARSE_STR_UNESC_CONTROL;
        }
        switch (**c) {
        case '"':
            (*c)++;
            v->type = JG_TYPE_STR;
            size_t byte_c = *c - v->json;
            if (byte_c > UINT32_MAX) {
                return JG_E_PARSE_STR_TOO_LARGE; // 4 billion chars is too much
            }
            v->byte_c = byte_c;
            return JG_OK;
        case '\\':
            switch (*++(*c)) {
            case 'b': case 't': case 'n': case 'f': case 'r': case '"':
            case '/': case '\\':
                continue;
            case 'u': // 4 UTF-16 code point encoding hex chars (e.g., "\uE90F")
                if (*++(*c) < '0' || **c > 'f' ||
                    (**c > '9' && **c < 'A') || (**c > 'F' && **c < 'a') ||
                    *++(*c) < '0' || **c > 'f' ||
                    (**c > '9' && **c < 'A') || (**c > 'F' && **c < 'a') ||
                    *++(*c) < '0' || **c > 'f' ||
                    (**c > '9' && **c < 'A') || (**c > 'F' && **c < 'a') ||
                    *++(*c) < '0' || **c > 'f' ||
                    (**c > '9' && **c < 'A') || (**c > 'F' && **c < 'a')) {
                    return JG_E_PARSE_STR_UTF16_INVALID;
                }
                // Check surrogate pair range: \uD800 through \uDFFF
                if (((*c)[-3] == 'D' || (*c)[-3] == 'd') && (*c)[-2] > '7') {
                    // Check low surrogate pair range: \uDC00 through \uDFFF
                    if ((*c)[-2] > 'B' && (*c)[-2] != 'a' && (*c)[-2] != 'b') {
                        return JG_E_PARSE_STR_UTF16_UNPAIRED_LOW;
                    }
                    // Verify that the high surrogate is followed by a low one.
                    if (*++(*c) != '/' ||
                        *++(*c) != 'u' ||
                        (*++(*c) != 'D' && **c != 'd') ||
                        *++(*c) < 'C' ||
                            **c > 'f' ||
                            (**c > 'F' && **c < 'c') ||
                        *++(*c) < '0' ||
                            **c > 'f' ||
                            (**c > '9' && **c < 'A') ||
                            (**c > 'F' && **c < 'a') ||
                        *++(*c) < '0' ||
                            **c > 'f' ||
                            (**c > '9' && **c < 'A') ||
                            (**c > 'F' && **c < 'a')) {
                        return JG_E_PARSE_STR_UTF16_UNPAIRED_HIGH;
                    }
                }
                // If none of the above 3 errors were returned, the escape
                // sequence encodes a valid unicode code point.
                continue;
            default:
                return JG_E_PARSE_STR_ESC_INVALID;
            }
        default:
            continue;
        }
    }
}

// parse_element() prototype needed here due to mutual recursion
static jg_ret parse_element(
    char const * * c,
    struct jg_val_in * v
);

static jg_ret parse_array(
    char const * * c,
    struct jg_val_in * v
) {
    char const * const open_bracket = (*c)++; // '['
    reskip_any_whitespace(c);
    if (**c == ']') {
        (*c)++;
        v->type = JG_TYPE_ARR;
        struct jg_arr * arr = malloc(sizeof(struct jg_arr));
        if (!arr) {
            return JG_E_MALLOC;
        }
        arr->json = open_bracket; 
        arr->elem_c = 0;
        v->arr = arr;
        return JG_OK;
    }
    size_t elem_c = 0;
    for (;;) {
        reskip_element(c);
        elem_c++;
        reskip_any_whitespace(c);
        switch (**c) {
        case ']':
            break;
        case ',':
            (*c)++;
            continue;
        default:
            return JG_E_PARSE_ARR_INVALID_SEP;
        }
        break;
    }
    v->type = JG_TYPE_ARR;
    struct jg_arr * arr = calloc(1,
        sizeof(struct jg_arr) + elem_c * sizeof(struct jg_val_in));
    if (!arr) {
        return JG_E_CALLOC;
    }
    arr->json = open_bracket;
    arr->elem_c = elem_c;
    v->arr = arr;
    *c = open_bracket + 1;
    for (struct jg_val_in * elem = arr->elems;
        elem < arr->elems + arr->elem_c; elem++) {
        JG_GUARD(parse_element(c, elem));
        reskip_any_whitespace(c);
        // Check needed here because the reskip_element() call above is actually
        // a little too greedy when the element type is number/true/false/null.
        if (**c != ',' && **c != ']') {
            return JG_E_PARSE_ARR_INVALID_SEP;
        }
        (*c)++; // skip the ',' or ']'
    }
    return JG_OK;
}

static jg_ret check_key_is_unique(
    struct jg_pair const * pairs,
    struct jg_pair const * pair,
    char const * key,
    uint32_t byte_c
) {
    // Technically, the JSON spec inexplicably allows duplicate keys; but
    // Jgrandson does not, primarily because pairs with duplicate keys would be
    // inaccessible with Jgrandson's getter API.
    for (struct jg_pair const * p = pairs; p < pair; p++) {
        bool strings_are_equal = false;
        JG_GUARD(json_strings_are_equal((uint8_t const *) p->key.json,
            p->key.byte_c, (uint8_t const *) key, byte_c, &strings_are_equal));
        if (strings_are_equal) {
            return JG_E_PARSE_OBJ_DUPLICATE_KEY;
        }
    }
    return JG_OK;
}

static jg_ret parse_object(
    char const * * c,
    struct jg_val_in * v
) {
    char const * const open_brace = (*c)++; // '{'
    reskip_any_whitespace(c);
    if (**c == '}') {
        (*c)++;
        v->type = JG_TYPE_OBJ;
        struct jg_obj * obj = malloc(sizeof(struct jg_obj));
        if (!obj) {
            return JG_E_MALLOC;
        }
        obj->json = open_brace;
        obj->pair_c = 0;
        v->obj = obj;
        return JG_OK;
    }
    size_t pair_c = 0;
    for (;;) {
        if (**c != '"') {
            return JG_E_PARSE_OBJ_INVALID_KEY;
        }
        reskip_string(c);
        reskip_any_whitespace(c);
        if (**c != ':') {
            return JG_E_PARSE_OBJ_KEYVAL_INVALID_SEP;
        }
        (*c)++;
        reskip_element(c);
        pair_c++;
        reskip_any_whitespace(c);
        switch (**c) {
        case '}':
            break;
        case ',':
            (*c)++;
            reskip_any_whitespace(c);
            continue;
        default:
            return JG_E_PARSE_OBJ_INVALID_SEP;
        }
        break;
    }
    v->type = JG_TYPE_OBJ;
    struct jg_obj * obj = calloc(1,
        sizeof(struct jg_obj) + pair_c * sizeof(struct jg_pair));
    if (!obj) {
        return JG_E_CALLOC;
    }
    obj->json = open_brace;
    obj->pair_c = pair_c;
    v->obj = obj;
    *c = open_brace + 1;
    for (struct jg_pair * p = obj->pairs; p < obj->pairs + obj->pair_c; p++) {
        reskip_any_whitespace(c);
        JG_GUARD(parse_string(c, &p->key));
        JG_GUARD(check_key_is_unique(obj->pairs, p, p->key.json,
            p->key.byte_c));
        reskip_any_whitespace(c);
        (*c)++; // skip the ':' already known to be here
        JG_GUARD(parse_element(c, &p->val));
        reskip_any_whitespace(c);
        // Check needed here because the reskip_element() call above is actually
        // a little too greedy when the element type is number/true/false/null.
        if (**c != ',' && **c != '}') {
            return JG_E_PARSE_OBJ_INVALID_SEP;
        }
        (*c)++; // skip the ',' or '}'
    }
    return JG_OK;
}

// Any value parsed by parse_element() has already been skipped over before, so
// all parse_...() calls below can be called safely without c_over checks.
static jg_ret parse_element(
    char const * * c,
    struct jg_val_in * v
) {
    reskip_any_whitespace(c);
    switch (**c) {
    case '"':
        return parse_string(c, v);
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return parse_number(c, NULL, v);
    case '[':
        return parse_array(c, v);
    case 'f':
        return parse_false(c, NULL, v);
    case 'n':
        return parse_null(c, NULL, v);
    case 't':
        return parse_true(c, NULL, v);
    case '{':
        return parse_object(c, v);
    default:
        return JG_E_PARSE_INVALID_TYPE;
    }
}

// According to https://tools.ietf.org/html/rfc8259 (the current JSON spec at
// the time of writing), any type of JSON value (i.e., string, number, array,
// object, true, false, or null) is also valid at the root level.
static jg_ret parse_root(
    jg_t * jg
) {
    jg->json_cur = jg->json_text;
    skip_any_whitespace(&jg->json_cur, jg->json_over);
    switch (*jg->json_cur) {
    case '"':
        // Make sure the JSON text root contains a complete string...
        JG_GUARD(skip_string(&jg->json_cur, jg->json_over));
        jg->json_cur = jg->json_text;
        // ...because parse_string() doesn't check jg->json_over.
        JG_GUARD(parse_string(&jg->json_cur, &jg->root_in));
        break;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
        JG_GUARD(parse_number(&jg->json_cur, jg->json_over, &jg->root_in));
        break;
    case '[':
        // make sure the JSON text root contains a complete array...
        JG_GUARD(skip_array(&jg->json_cur, jg->json_over));
        jg->json_cur = jg->json_text;
        // ...because parse_array() doesn't check jg->json_over.
        JG_GUARD(parse_array(&jg->json_cur, &jg->root_in));
        break;
    case 'f':
        JG_GUARD(parse_false(&jg->json_cur, jg->json_over, &jg->root_in));
        break;
    case 'n':
        JG_GUARD(parse_null(&jg->json_cur, jg->json_over, &jg->root_in));
        break;
    case 't':
        JG_GUARD(parse_true(&jg->json_cur, jg->json_over, &jg->root_in));
        break;
    case '{':
        // make sure the JSON text root contains a complete object...
        JG_GUARD(skip_object(&jg->json_cur, jg->json_over));
        jg->json_cur = jg->json_text;
        // ...because parse_array() doesn't check jg->json_over.
        JG_GUARD(parse_object(&jg->json_cur, &jg->root_in));
        break;
    default:
        return JG_E_PARSE_INVALID_TYPE;
    }
    skip_any_whitespace(&jg->json_cur, jg->json_over);
    if (jg->json_cur < jg->json_over) {
        return JG_E_PARSE_ROOT_SURPLUS;
    }
    jg->state = JG_STATE_GET;
    return JG_OK;
}

// Copy the JSON text string to a malloc-ed char buffer, then parse.
jg_ret jg_parse_str(
    jg_t * jg,
    char const * json_text, // null-terminator not required
    size_t byte_c // excluding null-terminator
) {
    if (jg->state != JG_STATE_INIT) {
        return jg->ret = JG_E_STATE_NOT_PARSE;
    }
    jg->state = JG_STATE_PARSE;
    JG_GUARD(alloc_strcpy(jg, &jg->json_text, json_text, byte_c));
    // Overwrite null-terminator with a newline. The parse functions do not
    // expect a null-terminator, and having a final whitespace char can help
    // avoid an unnecessary malloc() edge case in jg_root_get_<number_type>().
    jg->json_text[byte_c] = '\n';
    jg->json_over = jg->json_text + ++byte_c;
    return jg->ret = parse_root(jg);
}

// Parse the JSON text string without copying it to a malloc-ed char buffer.
jg_ret jg_parse_callerstr(
    jg_t * jg,
    char const * json_text, // null-terminator not required
    size_t byte_c // excluding null-terminator
) {
    if (jg->state != JG_STATE_INIT) {
        return jg->ret = JG_E_STATE_NOT_PARSE;
    }
    jg->state = JG_STATE_PARSE;
    jg->json_callertext = json_text;
    jg->json_is_callertext = true;
    jg->json_over = jg->json_callertext + byte_c;
    return jg->ret = parse_root(jg);
}

// Open file, copy contents to a malloc-ed char buffer, close file; then parse.
jg_ret jg_parse_file(
    jg_t * jg,
    char const * filepath
) {
    if (jg->state != JG_STATE_INIT) {
        return jg->ret = JG_E_STATE_NOT_PARSE;
    }
    jg->state = JG_STATE_PARSE;
    FILE * f = fopen(filepath, "r");
    if (!f) {
        jg->err_val.errn = errno;
        return jg->ret = JG_E_ERRNO_FOPEN;
    }
    if (fseeko(f, 0, SEEK_END) == -1) {
        jg->err_val.errn = errno;
        return jg->ret = JG_E_ERRNO_FSEEKO;
    }
    size_t byte_c = 0;
    {
        off_t size = ftello(f);
        if (size < 0) {
            jg->err_val.errn = errno;
            return jg->ret = JG_E_ERRNO_FTELLO;
        }
        byte_c = (size_t) size;
    }
    rewind(f);
    jg->json_text = malloc(byte_c + 1);
    // Append a newline instead of a null-terminator. The parse functions do not
    // expect a null-terminator, and having a final whitespace char can help
    // avoid an unnecessary malloc() edge case in jg_root_get_<number_type>().
    jg->json_text[byte_c] = '\n';
    if (!jg->json_text) {
        return jg->ret = JG_E_MALLOC;
    }
    if (fread(jg->json_text, 1, byte_c, f) != byte_c) {
        free(jg->json_text);
        return jg->ret = JG_E_FREAD;
    }
    if (fclose(f)) {
        free(jg->json_text);
        jg->err_val.errn = errno;
        return jg->ret = JG_E_ERRNO_FCLOSE;
    }
    jg->json_over = jg->json_text + ++byte_c;
    return jg->ret = parse_root(jg);
}
