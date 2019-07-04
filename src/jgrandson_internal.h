// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include "jgrandson.h"

// Is UTF-8 continuation byte (i.e., 0b10XXXXXX) ? 0 : 1
#define JG_IS_1ST_UTF8_BYTE(byte) (((byte) & 0xC0) != 0x80)

enum jg_state {
    JG_STATE_INIT = 0,
    JG_STATE_PARSE = 1,
    JG_STATE_GET = 2,
    JG_STATE_SET = 3,
    JG_STATE_GENERATE = 4
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
        uint32_t elem_c; // JG_TYPE_ARRAY -- number of jg_val elements
        uint32_t keyval_c; // JG_TYPE_OBJECT -- number of jg_keyval elements
    };
    enum jg_type type;
};

struct jg_keyval {
    struct jg_val key; // implemented as jg_val struct of type JG_TYPE_STRING
    struct jg_val val;
};

union jg_err_val {
    intmax_t i; // getter min/max int boundary errors
    uintmax_t u; // getter min/max uint boundary errors
    size_t s; // getter min_*_c/max_*_c element count boundary errors
    int errn; // errno set through external function calls
    enum jg_state state; // wrong state errors
    enum jg_type type; // getter type errors
};

struct jgrandson {
    struct jg_val root_val;
    uint8_t const * json_str; // the start of the JSON string to be parsed
    uint8_t const * json_cur; // current parsing position within json_str
    uint8_t const * json_over; // the byte following the end of the json_str buf
    char const * err_str; // ref to the string returned by jg_get_err_str()
    union jg_err_val err_expected;
    union jg_err_val err_received;
    jg_ret ret;
    enum jg_state state;
    bool json_str_needs_free;
    bool err_str_needs_free;
};

// non-API jg_util.c prototypes:

void free_json_str(
    jg_t * jg
);

void free_err_str(
    jg_t * jg
);

jg_ret check_state(
    jg_t * jg,
    enum jg_state state
);
