// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum jg_type {
    JG_TYPE_OBJECT = 0,
    JG_TYPE_ARRAY = 1,
    JG_TYPE_STRING = 2,
    JG_TYPE_NUMBER = 3,
    JG_TYPE_FALSE = 4,
    JG_TYPE_TRUE = 5,
    JG_TYPE_NULL = 6
};

// 8 or 4 byte pointer + uint32_t union + 4 byte enum(=int): 16 or 12 byte total
struct jg_val {
    union {
        uint8_t const * str; // JG_TYPE_STRING/NUMBER -- NOT null-terminated!
        struct jg_val * arr; // JG_TYPE_ARRAY -- array of val_c elements
        struct jg_keyval * obj; // JG_TYPE_OBJECT -- array of keyval_c elements
    };
    // A single "uint32_t size" for all types would of course work fine too, but
    // using the union below allows writing code that is more self-documenting.
    union {
        uint32_t byte_c; // JG_TYPE_STRING/NUMBER -- NO null-termination!
        uint32_t val_c; // JG_TYPE_ARRAY -- number of jg_val elements
        uint32_t keyval_c; // JG_TYPE_OBJECT -- number of jg_keyval elements
    };
    enum jg_type type;
};

struct jg_keyval {
    struct jg_val key; // implemented as jg_val struct of type JG_TYPE_STRING
    struct jg_val val;
};

#define JG_GUARD(jg_call) do { \
    char const * err_str = (jg_call); \
    if (err_str) { \
        return err_str; \
    } \
} while (0)

char const * jg_parse_str(
    char const * json_str,
    size_t byte_size,
    struct jg_val * v
);
