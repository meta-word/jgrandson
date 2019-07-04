// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#define _POSIX_C_SOURCE 201112L // ftello() and fseeko()'s off_t
#define _FILE_OFFSET_BITS 64 // Ensure off_t is 64 bits (possibly redundant)

#include "jgrandson_internal.h"

static void skip_any_whitespace(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    while (*u < u_over &&
        (**u == ' ' || **u == '\n' || **u == '\t' || **u == '\r')) {
        (*u)++;
    }
}

// For cases where it's already known that the JSON string buffer is long enough
static void reskip_any_whitespace(
    uint8_t const * * u
) {
    while (**u == ' ' || **u == '\n' || **u == '\t' || **u == '\r') {
        (*u)++;
    }
}

// Try to go to set *u to the char after the string's closing quotation mark,
// but return an error if u_over was reached before that mark was found.
static jg_ret skip_string(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    // *u assumed to point to the opening quotation mark.
    uint8_t const * const u_backup = *u;
    while (++(*u) < u_over) {
        if (**u == '"') {
            (*u)++;
            return JG_OK;
        }
        if (**u == '\\') {
            if (++(*u) >= u_over) {
                break;
            }
        }
    }
    *u = u_backup; // Set u to the opening " to provide it as error context.
    return JG_E_PARSE_UNTERM_STR;
}

// For strings that have previously gone through skip_string() successfully
static void reskip_string(
    uint8_t const * * u
) {
    // *u assumed to point to the opening quotation mark.
    while (*++(*u) != '"') {
        if (**u == '\\') {
            (*u)++;
        }
    }
    (*u)++;
}

// Try to go to set *u to the char after the array's closing bracket (']'),
// but return an error if u_over was reached before that bracket was found.
static jg_ret skip_array(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    // *u assumed to point to the opening bracket ('[').
    uint8_t const * const u_backup = (*u)++;
    while (*u < u_over) {
        switch (**u) {
        case '"':
            JG_GUARD(skip_string(u, u_over));
            continue;
        case '[':
            JG_GUARD(skip_array(u, u_over));
            continue;
        case ']':
            (*u)++;
            return JG_OK;
        default:
            (*u)++;
            continue;
        }
    }
    *u = u_backup; // Set u to the opening [ to provide it as error context.
    return JG_E_PARSE_UNTERM_ARR;
}

// For arrays that have previously been successfully skipped
static void reskip_array(
    uint8_t const * * u
) {
    // *u assumed to point to the opening bracket ('[').
    (*u)++;
    while (**u != ']') {
        switch (**u) {
        case '"':
            reskip_string(u);
            continue;
        case '[':
            reskip_array(u);
            continue;
        default:
            (*u)++;
            continue;
        }
    }
    (*u)++;
}

// Try to go to set *u to the char after the object's closing brace ('}'),
// but return an error if u_over was reached before that brace was found.
static jg_ret skip_object(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    // *u assumed to point to the opening brace ('{').
    uint8_t const * const u_backup = (*u)++;
    while (*u < u_over) {
        switch (**u) {
        case '"':
            JG_GUARD(skip_string(u, u_over));
            continue;
        case '{':
            JG_GUARD(skip_object(u, u_over));
            continue;
        case '}':
            (*u)++;
            return JG_OK;
        default:
            (*u)++;
            continue;
        }
    }
    *u = u_backup; // Set u to the opening { to provide it as error context.
    return JG_E_PARSE_UNTERM_OBJ;
}

// For objects that have previously been successfully skipped
static void reskip_object(
    uint8_t const * * u
) {
    // *u assumed to point to the opening brace ('{').
    (*u)++;
    while (**u != '}') {
        switch (**u) {
        case '"':
            reskip_string(u);
            continue;
        case '{':
            reskip_object(u);
            continue;
        default:
            (*u)++;
            continue;
        }
    }
    (*u)++;
}

// For elements that have previously been successfully skipped
static void reskip_element(
    uint8_t const * * u
) {
    reskip_any_whitespace(u);
    switch (**u) {
    case '"':
        reskip_string(u);
        return;
    case '[':
        reskip_array(u);
        return;
    case '{':
        reskip_object(u);
        return;
    default:
        while (**u != ',' && **u != ']' && **u != '}') {
            (*u)++; // reskip number/true/false/null/whitespace
        }
    }
}

static jg_ret parse_false(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) == u_over || **u != 'a' ||
        ++(*u) == u_over || **u != 'l' ||
        ++(*u) == u_over || **u != 's' ||
        ++(*u) == u_over || **u != 'e') {
        return JG_E_PARSE_FALSE;
    }
    (*u)++;
    v->type = JG_TYPE_FALSE;
    return JG_OK;
}

static jg_ret parse_true(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) == u_over || **u != 'r' ||
        ++(*u) == u_over || **u != 'u' ||
        ++(*u) == u_over || **u != 'e') {
        return JG_E_PARSE_TRUE;
    }
    (*u)++;
    v->type = JG_TYPE_TRUE;
    return JG_OK;
}

static jg_ret parse_null(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) == u_over || **u != 'u' ||
        ++(*u) == u_over || **u != 'l' ||
        ++(*u) == u_over || **u != 'l') {
        return JG_E_PARSE_NULL;
    }
    (*u)++;
    v->type = JG_TYPE_NULL;
    return JG_OK;
}

static jg_ret parse_number(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    uint8_t const * str = *u;
    if (**u == '-' && (++(*u) == u_over || **u < '0' || **u > '9')) {
        return JG_E_PARSE_NUM_SIGN;
    }
    if (**u == '0' && ++(*u) != u_over && **u >= '0' && **u <= '9') {
        return JG_E_PARSE_NUM_LEAD_ZERO;
    }
    bool has_decimal_point = false;
    do {
        switch (**u) {
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
            if (++(*u) == u_over ||
                ((**u == '+' || **u == '-') && ++(*u) == u_over) ||
                **u < '0' || **u > '9') {
                return JG_E_PARSE_NUM_EXP_HEAD_INVALID;
            }
            while (++(*u) != u_over) {
                switch (**u) {
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
    } while (++(*u) != u_over);
    number_parsed:
    v->type = JG_TYPE_NUM;
    v->str = str;
    v->byte_c = *u - str;
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
    uint8_t const * * u,
    struct jg_val * v
) {
    // *u assumed to point to the opening quotation mark.
    uint8_t const * const str = *u + 1;
    for (;;) {
        if (*++(*u) < ' ') {
            return JG_E_PARSE_STR_UNESC_CONTROL;
        }
        switch (**u) {
        case '"':
            (*u)++;
            v->type = JG_TYPE_STR;
            v->str = str;
            v->byte_c = *u - str;
            return JG_OK;
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
                    return JG_E_PARSE_STR_UTF16_INVALID;
                }
                // check surrogate pair range: \uD800 through \uDFFF
                if (((*u)[-3] == 'D' || (*u)[-3] == 'd') && (*u)[-2] > '7') {
                    // check high surrogate pair range: \uDC00 through \uDFFF
                    if ((*u)[-2] > 'B' && (*u)[-2] != 'a' && (*u)[-2] != 'b') {
                        return JG_E_PARSE_STR_UTF16_UNPAIRED_HIGH;
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
                        return JG_E_PARSE_STR_UTF16_UNPAIRED_LOW;
                    }
                }
                continue;
            default:
                return JG_E_PARSE_STR_ESC_INVALID;
            }
        default:
            if (**u > 0x7F) {
                JG_GUARD(parse_utf8(u));
            }
        }
    }
}

// parse_element() prototype needed here due to mutual recursion
static jg_ret parse_element(
    uint8_t const * * u,
    struct jg_val * v
);

static jg_ret parse_array(
    uint8_t const * * u,
    struct jg_val * v
) {
    // *u assumed to point to the opening bracket ('[').
    (*u)++;
    reskip_any_whitespace(u);
    if (**u == ']') {
        (*u)++;
        v->type = JG_TYPE_ARR;
        return JG_OK;
    }
    uint8_t const * const u_backup = *u;
    for (;;) {
        reskip_element(u);
        v->elem_c++;
        reskip_any_whitespace(u);
        switch (**u) {
        case ']':
            break;
        case ',':
            (*u)++;
            continue;
        default:
            return JG_E_PARSE_ARR_INVALID_SEP;
        }
        break;
    }
    v->type = JG_TYPE_ARR;
    v->arr = calloc(v->elem_c, sizeof(struct jg_val));
    if (!v->arr) {
        return JG_E_CALLOC;
    }
    *u = u_backup;
    for (struct jg_val * elem = v->arr; elem < v->arr + v->elem_c; elem++) {
        JG_GUARD(parse_element(u, elem));
        reskip_any_whitespace(u);
        // Check needed here because the reskip_element() call above is actually
        // a little too greedy when the element type is number/true/false/null.
        if (**u != ',' && **u != ']') {
            return JG_E_PARSE_ARR_INVALID_SEP;
        }
        (*u)++; // skip the ',' or ']'
    }
    return JG_OK;
}

static jg_ret parse_object(
    uint8_t const * * u,
    struct jg_val * v
) {
    // *u assumed to point to the opening brace ('{').
    (*u)++;
    reskip_any_whitespace(u);
    if (**u == '}') {
        (*u)++;
        v->type = JG_TYPE_OBJ;
        return JG_OK;
    }
    uint8_t const * const u_backup = *u;
    for (;;) {
        if (**u != '"') {
            return JG_E_PARSE_OBJ_INVALID_KEY;
        }
        reskip_string(u);
        reskip_any_whitespace(u);
        if (**u != ':') {
            return JG_E_PARSE_OBJ_KEYVAL_INVALID_SEP;
        }
        (*u)++;
        reskip_element(u);
        v->keyval_c++;
        reskip_any_whitespace(u);
        switch (**u) {
        case '}':
            break;
        case ',':
            (*u)++;
            reskip_any_whitespace(u);
            continue;
        default:
            return JG_E_PARSE_OBJ_INVALID_SEP;
        }
        break;
    }
    v->type = JG_TYPE_OBJ;
    v->obj = calloc(v->keyval_c, sizeof(struct jg_keyval));
    if (!v->obj) {
        return JG_E_CALLOC;
    }
    *u = u_backup;
    for (struct jg_keyval * kv = v->obj; kv < v->obj + v->keyval_c; kv++) {
        reskip_any_whitespace(u);
        JG_GUARD(parse_string(u, &kv->key));
        reskip_any_whitespace(u);
        (*u)++; // skip the ':' already known to be here
        JG_GUARD(parse_element(u, &kv->val));
        reskip_any_whitespace(u);
        // Check needed here because the reskip_element() call above is actually
        // a little too greedy when the element type is number/true/false/null.
        if (**u != ',' && **u != '}') {
            return JG_E_PARSE_OBJ_INVALID_SEP;
        }
        (*u)++; // skip the ',' or '}'
    }
    return JG_OK;
}

// Any value parsed by parse_element() has already been skipped over before, so
// all parse_...() calls below can be called safely without u_over checks.
static jg_ret parse_element(
    uint8_t const * * u,
    struct jg_val * v
) {
    reskip_any_whitespace(u);
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
        return JG_E_PARSE_INVALID_TYPE;
    }
}

// According to https://tools.ietf.org/html/rfc8259 (the current JSON spec at
// the time of writing), any type of JSON value (i.e., string, number, array,
// object, true, false, or null) is also valid at the root level.
static jg_ret parse_root(
    jg_t * jg
) {
    jg->json_cur = jg->json_str;
    skip_any_whitespace(&jg->json_cur, jg->json_over);
    switch (*jg->json_cur) {
    case '"':
        // Make sure the JSON text root contains a complete string...
        JG_GUARD(skip_string(&jg->json_cur, jg->json_over));
        jg->json_cur = jg->json_str;
        // ...because parse_string() doesn't check jg->json_over.
        JG_GUARD(parse_string(&jg->json_cur, &jg->root_val));
        break;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
        JG_GUARD(parse_number(&jg->json_cur, jg->json_over, &jg->root_val));
        break;
    case '[':
        // make sure the JSON text root contains a complete array...
        JG_GUARD(skip_array(&jg->json_cur, jg->json_over));
        jg->json_cur = jg->json_str;
        // ...because parse_array() doesn't check jg->json_over.
        JG_GUARD(parse_array(&jg->json_cur, &jg->root_val));
        break;
    case 'f':
        JG_GUARD(parse_false(&jg->json_cur, jg->json_over, &jg->root_val));
        break;
    case 'n':
        JG_GUARD(parse_null(&jg->json_cur, jg->json_over, &jg->root_val));
        break;
    case 't':
        JG_GUARD(parse_true(&jg->json_cur, jg->json_over, &jg->root_val));
        break;
    case '{':
        // make sure the JSON text root contains a complete object...
        JG_GUARD(skip_object(&jg->json_cur, jg->json_over));
        jg->json_cur = jg->json_str;
        // ...because parse_array() doesn't check jg->json_over.
        JG_GUARD(parse_object(&jg->json_cur, &jg->root_val));
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

jg_ret jg_parse_str(
    jg_t * jg,
    char const * json_str,
    size_t byte_c
) {
    JG_GUARD(check_state(jg, JG_STATE_INIT));
    jg->state = JG_STATE_PARSE;
    free_json_str(jg);
    uint8_t * json_str_copy = malloc(byte_c);
    if (!json_str_copy) {
        return jg->ret = JG_E_MALLOC;
    }
    memcpy(json_str_copy, json_str, byte_c);
    jg->json_str = json_str_copy;
    jg->json_str_needs_free = true;
    jg->json_over = jg->json_str + byte_c;
    return jg->ret = parse_root(jg);
}

jg_ret jg_parse_str_no_copy(
    jg_t * jg,
    char const * json_str,
    size_t byte_c
) {
    JG_GUARD(check_state(jg, JG_STATE_INIT));
    jg->state = JG_STATE_PARSE;
    free_json_str(jg);
    jg->json_str = (uint8_t const *) json_str;
    jg->json_over = jg->json_str + byte_c;
    return jg->ret = parse_root(jg);
}

jg_ret jg_parse_file(
    jg_t * jg,
    char const * filepath
) {
    JG_GUARD(check_state(jg, JG_STATE_INIT));
    jg->state = JG_STATE_PARSE;
    free_json_str(jg);
    FILE * f = fopen(filepath, "r");
    if (!f) {
        jg->err_received.errn = errno;
        return jg->ret = JG_E_ERRNO_FOPEN;
    }
    if (fseeko(f, 0, SEEK_END) == -1) {
        jg->err_received.errn = errno;
        return jg->ret = JG_E_ERRNO_FSEEKO;
    }
    size_t byte_c = 0;
    {
        off_t size = ftello(f);
        if (size < 0) {
            jg->err_received.errn = errno;
            return jg->ret = JG_E_ERRNO_FTELLO;
        }
        byte_c = (size_t) size;
    }
    rewind(f);
    uint8_t * json_str = malloc(byte_c);
    if (!json_str) {
        return jg->ret = JG_E_MALLOC;
    }
    if (fread(json_str, 1, byte_c, f) != byte_c) {
        free(json_str);
        return jg->ret = JG_E_FREAD;
    }
    if (fclose(f)) {
        free(json_str);
        jg->err_received.errn = errno;
        return jg->ret = JG_E_ERRNO_FCLOSE;
    }
    jg->json_str = json_str;
    jg->json_str_needs_free = true;
    jg->json_over = jg->json_str + byte_c;
    return jg->ret = parse_root(jg);
}
