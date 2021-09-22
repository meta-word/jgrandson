// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jgrandson_internal.h"

// Unlike the other files, jg_unicode.c expects string pointers to be of type
// (uint8_t *) instead of (char *) for ease of unsigned arithmetic on non-ASCII.

bool is_utf8_continuation_byte(
    uint8_t u
) {
    // True if 0b10XXXXXX, otherwise false.
    return (u & 0xC0) == 0x80;
}

size_t get_utf8_char_size(
    uint8_t const * u,
    uint8_t const * const u_over
) {
    if (u >= u_over) return 0;
    if (++u == u_over || !is_utf8_continuation_byte(*u)) return 1;
    if (++u == u_over || !is_utf8_continuation_byte(*u)) return 2;
    return ++u == u_over || !is_utf8_continuation_byte(*u) ? 3 : 4;
}

static uint32_t utf16hex_substr_to_codepoint( // E.g., "5F6A" to 0x5F6A
    uint8_t const * u // an already validated JSON substring
) {
    return 0x1000 * (u[0] - '0' - 7 * (u[0] > '9') - ((u[0] > 'F') << 5)) +
            0x100 * (u[1] - '0' - 7 * (u[1] > '9') - ((u[1] > 'F') << 5)) +
             0x10 * (u[2] - '0' - 7 * (u[2] > '9') - ((u[2] > 'F') << 5)) +
                    (u[3] - '0' - 7 * (u[3] > '9') - ((u[3] > 'F') << 5));
}

size_t get_unesc_byte_c(
    uint8_t const * const json_str, // an already validated JSON string
    size_t json_byte_c
) {
    size_t unesc_byte_c = 0;
    for (uint8_t const * u = json_str; u < json_str + json_byte_c;) {
        if (*u++ != '\\' || *u++ != 'u') {
            unesc_byte_c++;
            continue;
        }
        uint32_t codepoint = utf16hex_substr_to_codepoint(u);
        u += 4;
        if (codepoint - 0xD800U < 0x400) { // if UTF-16 surrogate pair
            u += 6; // Low surrogate already confirmed by parse_string()
            unesc_byte_c += 4; // To be encoded as a 4 byte UTF-8 character
        } else if (!codepoint) {
            ; // Jgrandson simply strips any embedded null-terminators.
        } else if (codepoint < 0x80) {
            unesc_byte_c++; // To be encoded as a 1 byte UTF-8 character
        } else if (codepoint < 0x800) {
            unesc_byte_c += 2; // To be encoded as a 2 byte UTF-8 character
        } else {
            unesc_byte_c += 3; // To be encoded as a 3 byte UTF-8 character
        }
    }
    return unesc_byte_c;
}

// Assumes that only characters that must be escaped will be escaped.
size_t get_json_byte_c(
    uint8_t const * const unesc_str,
    size_t unesc_byte_c
) {
    size_t json_byte_c = unesc_byte_c;
    for (uint8_t const * u = unesc_str; u < unesc_str + unesc_byte_c; u++) {
        switch (*u) {
        case '\b': case '\t': case '\n': case '\f': case '\r': case '"':
        case '\\':
            // Replaced by a 2 char sequence; e.g., line feed -> backslash and n
            json_byte_c++;
            continue;
        default:
            // rfc8259: control chars (U+0000 through 0+001F) must be escaped.
            if (*u < 0x20) {
                // Replaced by a 6 char sequence; e.g., bell ('\a') -> "\u0007"
                json_byte_c += 5;
            }
        }
    }
    return json_byte_c;
}

size_t get_codepoint_c(
    uint8_t const * const json_str, // an already validated JSON string
    size_t json_byte_c
) {
    size_t codepoint_c = 0;
    for (uint8_t const * u = json_str; u < json_str + json_byte_c;) {
        if (is_utf8_continuation_byte(*u)) {
            u++;
        } else {
            codepoint_c++;
            // Use of logical AND results in correct backslash escape increment.
            if (*u++ == '\\' && *u++ == 'u') {
                // Skip 4 hex digit chars, plus another 6 if a surrogate pair.
                u += utf16hex_substr_to_codepoint(u) - 0xD800U < 0x400 ? 10 : 4;
            }
        }
    }
    return codepoint_c;
}

void json_str_to_unesc_str(
    uint8_t const * const json_str, // an already validated JSON string
    size_t json_byte_c,
    uint8_t * unesc_str // pre-allocated (assisted by get_unesc_byte_c())
) {
    for (uint8_t const * u = json_str; u < json_str + json_byte_c;) {
        if (*u != '\\') {
            *unesc_str++ = *u++;
            continue;
        }
        u++;
        switch (*u++) {
            case  '"': *unesc_str++ =  '"'; continue;
            case '\\': *unesc_str++ = '\\'; continue;
            case  '/': *unesc_str++ =  '/'; continue;
            case  'b': *unesc_str++ = '\b'; continue;
            case  'f': *unesc_str++ = '\f'; continue;
            case  'n': *unesc_str++ = '\n'; continue;
            case  'r': *unesc_str++ = '\r'; continue;
            case  't': *unesc_str++ = '\t'; continue;
            case  'u': default: break; 
        }
        uint32_t codepoint = utf16hex_substr_to_codepoint(u);
        u += 4;
        uint32_t high_surrogate_payload = codepoint - 0xD800U;
        if (high_surrogate_payload < 0x400) { // Is this is a surrogate pair?
            u += 2;
            codepoint = 0x10000U + 0x400U * high_surrogate_payload +
                utf16hex_substr_to_codepoint(u) - 0xDC00U; // low surro payload
            u += 4;
            // Codepoints above 0x10000 need to be encoded as 4-byte UTF-8:
            *unesc_str++ = 0xF0 + codepoint / 0x40000;
            *unesc_str++ = 0x80 + codepoint / 0x1000 % 0x40;
            *unesc_str++ = 0x80 + codepoint / 0x40 % 0x40;
            *unesc_str++ = 0x80 + codepoint % 0x40;
        } else if (!codepoint) {
            ; // Jgrandson simply strips any embedded null-terminators.
        } else if (codepoint < 0x80) { // If true, encode as 1-byte UTF-8
            *unesc_str++ = codepoint;
        } else if (codepoint < 0x800) { // If true, encode as 2-byte UTF-8
            *unesc_str++ = 0xC0 + codepoint / 0x40;
            *unesc_str++ = 0x80 + codepoint % 0x40;
        } else { // Encode as 3-byte UTF-8
            *unesc_str++ = 0xE0 + codepoint / 0x1000;
            *unesc_str++ = 0x80 + codepoint / 0x40 % 0x40;
            *unesc_str++ = 0x80 + codepoint % 0x40;
        }
    }
}

// Only escapes chars that must be escaped in JSON strings according to rfc8259.
void unesc_str_to_json_str(
    uint8_t const * unesc_str,
    size_t unesc_byte_c,
    uint8_t * json_str // pre-allocated (assisted by get_json_byte_c())
) {
    for (uint8_t const * u = unesc_str; u < unesc_str + unesc_byte_c; u++) {
        switch (*u) {
        case '\b': *json_str++ = '\\'; *json_str++ = 'b' ; continue;
        case '\t': *json_str++ = '\\'; *json_str++ = 't' ; continue;
        case '\n': *json_str++ = '\\'; *json_str++ = 'n' ; continue;
        case '\f': *json_str++ = '\\'; *json_str++ = 'f' ; continue;
        case '\r': *json_str++ = '\\'; *json_str++ = 'r' ; continue;
        case '"' : *json_str++ = '\\'; *json_str++ = '"' ; continue;
        case '\\': *json_str++ = '\\'; *json_str++ = '\\'; continue;
        default:
            if (*u >= 0x20) {
                *json_str++ = *u;
                continue;
            }
        }
        // *u is a control character other than any of the cases above, so it
        // must be "represented as a six-character sequence: a reverse solidus,
        // followed by the lowercase letter u, followed by four hexadecimal
        // digits that encode the character's code point." (rfc8259)
        *json_str++ = '\\';
        *json_str++ = 'u';
        *json_str++ = '0';
        *json_str++ = '0';
        // Write high 4 bits as hex '0' or '1'; e.g., 0x0E -> '0', 0x19 -> '1'
        *json_str++ = '0' + (*u >> 4); // same as *u >= 0x10 ? '1' : '0'
        // Write low 4 bits as a hex digit char; e.g., 0x15 -> '5', 0x0C -> 'C'
        uint8_t low_half = *u & 0x0F;
        *json_str++ = '0' + low_half + 7 * (low_half > 0x09);
    }
}

jg_ret json_strings_are_equal(
    uint8_t const * const j1_str, // an already validated JSON string
    size_t j1_byte_c,
    uint8_t const * const j2_str, // an already validated JSON string
    size_t j2_byte_c,
    bool * strings_are_equal
) {
    // Try to compare the strings naively first...
    for (uint8_t const * u1 = j1_str, * u2 = j2_str;;) {
        if (u1 == j1_str + j1_byte_c) {
            *strings_are_equal = u2 == j2_str + j2_byte_c;
            return JG_OK;
        }
        if (u2 == j2_str + j2_byte_c) {
            *strings_are_equal = false;
            return JG_OK;
        }
        if (*u1 == '\\' || *u2 == '\\') {
            break; // Escaped string detected: abandon this approach.
        }
        if (*u1++ != *u2++) {
            *strings_are_equal = false;
            return JG_OK;
        }
    }
    size_t unesc_byte_c = get_unesc_byte_c(j1_str, j1_byte_c);
    if (get_unesc_byte_c(j2_str, j2_byte_c) != unesc_byte_c) {
        *strings_are_equal = false;
        return JG_OK;
    }
    // Using malloc() here is arguably an ugly and slow solution compared to
    // doing an in-place codepoint comparison, but that would involve writing
    // significantly more code for this cold branch. Maybe some other time.
    uint8_t * str_pair = malloc(2 * unesc_byte_c);
    if (!str_pair) {
        return JG_E_MALLOC;
    }
    json_str_to_unesc_str(j1_str, j1_byte_c, str_pair);
    json_str_to_unesc_str(j2_str, j2_byte_c, str_pair + unesc_byte_c);
    *strings_are_equal = !strncmp((char const *) str_pair,
        (char const *) (str_pair + unesc_byte_c), unesc_byte_c);
    free(str_pair);
    return JG_OK;
}

jg_ret unesc_str_and_json_str_are_equal(
    uint8_t const * const unesc_str,
    size_t unesc_byte_c,
    uint8_t const * const json_str, // an already validated JSON string
    size_t json_byte_c,
    bool * strings_are_equal
) {
    // Try to compare the strings naively first...
    for (uint8_t const * unesc = unesc_str, * json = json_str;;) {
        if (unesc == unesc_str + unesc_byte_c) {
            *strings_are_equal = unesc_byte_c == json_byte_c;
            return JG_OK;
        }
        if (json == json_str + json_byte_c) {
            *strings_are_equal = false;
            return JG_OK;
        }
        if (*json == '\\') {
            break; // Escaped string detected: abandon this approach.
        }
        if (*unesc++ != *json++) {
            *strings_are_equal = false;
            return JG_OK;
        }
    }
    if (get_unesc_byte_c(json_str, json_byte_c) != unesc_byte_c) {
        *strings_are_equal = false;
        return JG_OK;
    }
    // Using malloc() here is arguably an ugly and slow solution compared to
    // doing an in-place codepoint comparison, but that would involve writing
    // significantly more code for this cold branch. Maybe some other time.
    uint8_t * str = malloc(unesc_byte_c);
    if (!str) {
        return JG_E_MALLOC;
    }
    json_str_to_unesc_str(json_str, json_byte_c, str);
    *strings_are_equal = !strncmp((char *) str, (char const *) unesc_str,
        unesc_byte_c);
    free(str);
    return JG_OK;
}

#if defined(_WIN32) || defined(_WIN64)
wchar_t * str_to_wstr(
    char const * str
) { // The returned wstr must be free()d by the called after use.
    int wchar_c = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    wchar_t * wstr = malloc(wchar_c * sizeof(wchar_t)); // Includes space for ¥0
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, wchar_c);
    return wstr;
}
#endif