// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jg_skip.h"

void skip_any_whitespace(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    for (; *u < u_over; (*u)++) {
        if (**u != ' ' && **u != '\n' && **u != '\t' && **u != '\r') {
            return;
        }
    }
}

// The next 3 skip_X() functions increment the *u pointer until either of
// the following conditions occur:
//
// (1) A character marking the termination of X is encountered; after which
//     *u is incremented the remaining bytes needed to point to the first
//     non-whitespace character after X, if any. Then return "NULL".
//
// (2) *u reaches *u_over; i.e., the end of the JSON string is reached
//     before condition (1) occurs, indicating that X is unterminated.
//     Return an error message string regarding the unterminated character.
//
// Note that no JSON values are validated here. Validation occurs in parse_X()
// in jg_parse.c

// skip_string() merely attempts to increment *u until it finds a terminating
// double-quote. Given that the bit-representations of single-byte UTF-8
// octets (0x00 - 0x7F) and those of multi-byte UTF-8 octets (0x80 - 0xF4) are
// mutually exlusive, it does not need to be UTF-8-aware. It does however need
// to skip escaped characters and escaped UTF-16 hex encodings, the values of
// which can be equal to that of a double-quote (i.e., 0x22).
char const * skip_string(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    uint8_t const * const u_backup = *u;
    while (++(*u) < u_over) {
        if (**u == '"') {
            (*u)++;
            skip_any_whitespace(u, u_backup);
            return NULL;
        }
        if (**u == '\\') {
            if (++(*u) >= u_over) {
                break;
            }
            if (**u == 'u') {
                // Skip 4 UTF-16 code point encoding hex bytes
                if ((*u) + 4 >= u_over) {
                    break;
                }
                (*u) += 4;
            }
        }
    }
    *u = u_backup; // Set u to the opening " to provide it as error context.
    static char const e[] =
        "Unterminated string: closing double-quote ('\"') not found.";
    return e;
}

char const * skip_array(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    uint8_t const * const u_backup = *u;
    // If 1st iteration skip {, else skip obj/num/true/false/null/whitespace
    while (++(*u) < u_over) {
        switch (**u) {
        case '"':
            JG_GUARD(skip_string(u, u_over));
            continue;
        case '[':
            JG_GUARD(skip_array(u, u_over));
            continue;
        case ']':
            (*u)++;
            skip_any_whitespace(u, u_over);
            return NULL;
        default:
            continue;
        }
    }
    *u = u_backup; // Set u to the opening [ to provide it as error context.
    static char const e[] =
        "Unterminated array: closing square bracket (']') not found.";
    return e;
}

char const * skip_object(
    uint8_t const * * u,
    uint8_t const * const u_over
) {
    uint8_t const * const u_backup = *u;
    // If 1st iteration skip {, else skip arr/num/true/false/null/whitespace
    while (++(*u) < u_over) {
        switch (**u) {
        case '"':
            JG_GUARD(skip_string(u, u_over));
            continue;
        case '{':
            JG_GUARD(skip_object(u, u_over));
            continue;
        case '}':
            (*u)++;
            skip_any_whitespace(u, u_over);
            return NULL;
        default:
            continue;
        }
    }
    *u = u_backup; // Set u to the opening { to provide it as error context.
    static char const e[] =
        "Unterminated object: closing curly brace ('}') not found.";
    return e;
}

// Any value skipped by skip_element() has already been skipped over before, so
// all skip_...() calls below can be called safely without u_over checks.
void skip_element(
    uint8_t const * * u
) {
    skip_any_whitespace(u, NULL);
    switch (**u) {
    case '"':
        skip_string(u, NULL);
        return;
    case '[':
        skip_array(u, NULL);
        return;
    case '{':
        skip_object(u, NULL);
        return;
    default:
        while (**u != ',' && **u != ']' && **u != '}') {
            // skip number/true/false/null/whitespace
            (*u)++;
        }
    }
}
