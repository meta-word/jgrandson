// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jgrandson_internal.h"

bool is_utf8_continuation_byte(
    char c
) {
    // True if 0b10XXXXXX, otherwise false.
    return ((uint8_t) c & 0xC0) == 0x80;
}

size_t get_utf8_char_size(
    char const * c,
    char const * const c_over
) {
    if (c >= c_over) return 0;
    if (++c == c_over || !is_utf8_continuation_byte(*c)) return 1;
    if (++c == c_over || !is_utf8_continuation_byte(*c)) return 2;
    return ++c == c_over || !is_utf8_continuation_byte(*c) ? 3 : 4;
}
