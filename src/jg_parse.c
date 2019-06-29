// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#define _POSIX_C_SOURCE 201112L // ftello() and fseeko()'s off_t
#define _FILE_OFFSET_BITS 64 // Ensure off_t is 64 bits (possibly redundant)

#include "jg_skip.h"
#include "jg_util.h"

static jg_ret parse_false(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) >= u_over || **u != 'a' ||
        ++(*u) >= u_over || **u != 'l' ||
        ++(*u) >= u_over || **u != 's' ||
        ++(*u) >= u_over || **u != 'e') {
        return JG_E_PARSE_FALSE;
    }
    v->type = JG_TYPE_FALSE;
    return JG_OK;
}

static jg_ret parse_true(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) >= u_over || **u != 'r' ||
        ++(*u) >= u_over || **u != 'u' ||
        ++(*u) >= u_over || **u != 'e') {
        return JG_E_PARSE_TRUE;
    }
    v->type = JG_TYPE_TRUE;
    return JG_OK;
}

static jg_ret parse_null(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    if (++(*u) >= u_over || **u != 'u' ||
        ++(*u) >= u_over || **u != 'l' ||
        ++(*u) >= u_over || **u != 'l') {
        return JG_E_PARSE_NULL;
    }
    v->type = JG_TYPE_NULL;
    return JG_OK;
}

static jg_ret parse_number(
    uint8_t const * * u,
    uint8_t const * const u_over,
    struct jg_val * v
) {
    uint8_t const * str = *u;
    if (**u == '-' && (++(*u) >= u_over || **u < '0' || **u > '9')) {
        return JG_E_PARSE_NUM_SIGN;
    }
    if (**u == '0' && ++(*u) < u_over && **u >= '0' && **u <= '9') {
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
            if (++(*u) >= u_over ||
                ((**u == '+' || **u == '-') && ++(*u) >= u_over) ||
                **u < '0' || **u > '9') {
                return JG_E_PARSE_NUM_EXP_NO_DIGIT;
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
                    return JG_E_PARSE_NUM_EXP_INVALID;
                }
            }
            goto number_parsed;
        default:
            return JG_E_PARSE_NUM_INVALID;
        }
    } while (++(*u) < u_over);
    number_parsed:
    v->type = JG_TYPE_NUMBER;
    v->str = str;
    v->byte_c = *u - str;
    return JG_OK;
}

static jg_ret parse_utf8(
    uint8_t const * * u
) {
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
    return JG_E_PARSE_STR_UTF8_INVALID;
}

// parse_element() prototype needed here due to mutual recursion
static jg_ret parse_element(
    uint8_t const * * u,
    struct jg_val * v
);

static jg_ret parse_string(
    uint8_t const * * u,
    struct jg_val * v
) {
    uint8_t const * const str = *u + 1;
    for (;;) {
        if (*++(*u) < ' ') {
            return JG_E_PARSE_STR_UNESC_CONTROL;
        }
        switch (**u) {
        case '"':
            v->type = JG_TYPE_STRING;
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

static jg_ret parse_array(
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
        return JG_E_PARSE_ARR_INVALID_SEP;
    }
    v->type = JG_TYPE_ARRAY;
    if (!v->val_c) {
        return JG_OK;
    }
    *u = u_backup;
    v->arr = calloc(v->val_c, sizeof(struct jg_val));
    if (!v->arr) {
        return JG_E_CALLOC;
    }
    for (struct jg_val * elem = v->arr; elem < v->arr + v->val_c; elem++) {
        (*u)++; // skip '[' or ',' (already checked, so must return true)
        JG_GUARD(parse_element(u, elem));
        (*u)++; // skip last character of the element parsed
        skip_any_whitespace(u, NULL);
    }
    return JG_OK;
}

static jg_ret parse_object(
    uint8_t const * * u,
    struct jg_val * v
) {
    uint8_t const * const u_backup = *u;
    do {
        (*u)++;
        skip_any_whitespace(u, NULL);
        if (**u != '"') {
            return JG_E_PARSE_OBJ_INVALID_KEY;
        }
        skip_string(u, NULL);
        if (**u != ':') {
            return JG_E_PARSE_OBJ_KEYVAL_INVALID_SEP;
        }
        (*u)++;
        skip_any_whitespace(u, NULL);
        skip_element(u);
        v->keyval_c++;
    } while (**u == ',');
    if (**u != '}') {
        return JG_E_PARSE_OBJ_INVALID_SEP;
    }
    v->type = JG_TYPE_OBJECT;
    if (!v->keyval_c) {
        return JG_OK;
    }
    *u = u_backup;
    v->obj = calloc(v->keyval_c, sizeof(struct jg_keyval));
    if (!v->obj) {
        return JG_E_CALLOC;
    }
    for (struct jg_keyval * kv = v->obj; kv < v->obj + v->keyval_c; kv++) {
        (*u)++; // skip '{' or ',' (already checked, so must return true)
        JG_GUARD(parse_string(u, &kv->key));
        (*u)++; // skip ':' (already checked, so must return true)
        JG_GUARD(parse_element(u, &kv->val));
        (*u)++; // skip last character of the value element parsed
        skip_any_whitespace(u, NULL);
    }
    return JG_OK;
}

// Any value parsed by parse_element() has already been skipped over before, so
// all parse_...() calls below can be called safely without u_over checks.
static jg_ret parse_element(
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
        return JG_E_PARSE_INVALID_TYPE;
    }
}

// According to https://tools.ietf.org/html/rfc8259 (the current JSON spec at
// the time of writing), any type of JSON value (i.e., string, number, array,
// object, true, false, or null) is also valid at the root level.
static jg_ret parse_root(
    jg_t * jg,
    size_t byte_size
) {
    jg->json_cur = jg->json_str;
    uint8_t const * const u_over = jg->json_cur + byte_size;
    skip_any_whitespace(&jg->json_cur, u_over);
    switch (*jg->json_cur) {
    case '"':
        // Make sure the JSON string contains a complete string type...
        JG_GUARD(skip_string(&jg->json_cur, u_over));
        jg->json_cur = jg->json_str;
        // ...because parse_string() doesn't check u_over.
        return parse_string(&jg->json_cur, &jg->root_val);
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
        return parse_number(&jg->json_cur, u_over, &jg->root_val);
    case '[':
        // make sure the JSON string contains a complete root array...
        JG_GUARD(skip_array(&jg->json_cur, u_over));
        jg->json_cur = jg->json_str;
        // ...because parse_array() doesn't check u_over.
        return parse_array(&jg->json_cur, &jg->root_val);
    case 'f':
        return parse_false(&jg->json_cur, u_over, &jg->root_val);
    case 'n':
        return parse_null(&jg->json_cur, u_over, &jg->root_val);
    case 't':
        return parse_true(&jg->json_cur, u_over, &jg->root_val);
    case '{':
        // make sure the JSON string contains a complete root object...
        JG_GUARD(skip_object(&jg->json_cur, u_over));
        jg->json_cur = jg->json_str;
        // ...because parse_array() doesn't check u_over.
        return parse_object(&jg->json_cur, &jg->root_val);
    default:
        return JG_E_PARSE_INVALID_TYPE;
    }
}

jg_ret jg_parse_str(
    jg_t * jg,
    char const * json_str,
    size_t byte_size
) {
    free_json_str(jg);
    jg->json_str = malloc(byte_size);
    if (!jg->json_str) {
        return jg->ret = JG_E_MALLOC;
    }
    memcpy(jg->json_str, json_str, byte_size);
    jg->json_str_needs_free = true;
    return jg->ret =  parse_root(jg, byte_size);
}

jg_ret jg_parse_str_no_copy(
    jg_t * jg,
    char const * json_str,
    size_t byte_size
) {
    free_json_str(jg);
    jg->json_str = (uint8_t *) json_str;
    return jg->ret =  parse_root(jg, byte_size);
}

jg_ret jg_parse_file(
    jg_t * jg,
    char const * filepath
) {
    free_json_str(jg);
    FILE * f = fopen(filepath, "r");
    if (!f) {
        jg->errnum = errno;
        return jg->ret = JG_E_ERRNO_FOPEN;
    }
    if (fseeko(f, 0, SEEK_END) == -1) {
        jg->errnum = errno;
        return jg->ret = JG_E_ERRNO_FSEEKO;
    }
    size_t byte_size = 0;
    {
        off_t size = ftello(f);
        if (size < 0) {
            jg->errnum = errno;
            return jg->ret = JG_E_ERRNO_FTELLO;
        }
        byte_size = (size_t) size;
    }
    rewind(f);
    jg->json_str = malloc(byte_size);
    if (!jg->json_str) {
        return jg->ret = JG_E_MALLOC;
    }
    if (fread(jg->json_str, 1, byte_size, f) != byte_size) {
        return jg->ret = JG_E_FREAD;
    }
    if (fclose(f)) {
        jg->errnum = errno;
        return jg->ret = JG_E_ERRNO_FCLOSE;
    }
    jg->json_str_needs_free = true;
    return jg->ret =  parse_root(jg, byte_size);
}
