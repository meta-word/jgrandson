// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct jg_opaque jg_t;

typedef enum {
    JG_OK = 0,
    JG_E_MALLOC = 1,
    JG_E_CALLOC = 2,
    JG_E_NEWLOCALE = 3,
    JG_E_FREAD = 4,
    JG_E_ERRNO_FOPEN = 5,
    JG_E_ERRNO_FCLOSE = 6,
    JG_E_ERRNO_FSEEKO = 7,
    JG_E_ERRNO_FTELLO = 8,
    JG_E_PARSE_INVALID_TYPE = 9,
    JG_E_PARSE_UNTERM_STR = 10,
    JG_E_PARSE_UNTERM_ARR = 11,
    JG_E_PARSE_UNTERM_OBJ = 12,
    JG_E_PARSE_FALSE = 13,
    JG_E_PARSE_TRUE = 14,
    JG_E_PARSE_NULL = 15,
    JG_E_PARSE_NUM_SIGN = 16,
    JG_E_PARSE_NUM_LEAD_ZERO = 17,
    JG_E_PARSE_NUM_INVALID = 18,
    JG_E_PARSE_NUM_MULTIPLE_POINTS = 19,
    JG_E_PARSE_NUM_EXP_NO_DIGIT = 20,
    JG_E_PARSE_NUM_EXP_INVALID = 21,
    JG_E_PARSE_STR_UTF8_INVALID = 22,
    JG_E_PARSE_STR_UNESC_CONTROL = 23,
    JG_E_PARSE_STR_ESC_INVALID = 24,
    JG_E_PARSE_STR_UTF16_INVALID = 25,
    JG_E_PARSE_STR_UTF16_UNPAIRED_HIGH = 26,
    JG_E_PARSE_STR_UTF16_UNPAIRED_LOW = 27,
    JG_E_PARSE_ARR_INVALID_SEP = 28,
    JG_E_PARSE_OBJ_INVALID_KEY = 29,
    JG_E_PARSE_OBJ_KEYVAL_INVALID_SEP = 30,
    JG_E_PARSE_OBJ_INVALID_SEP = 31
} jg_ret;

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
    jg_ret ret = (jg_call); \
    if (ret != JG_OK) { \
        return ret; \
    } \
} while (0)

jg_t * jg_init(
    void
);

void jg_free(
    jg_t * jg
);

char const * jg_get_err_str(
    jg_t * jg
);

jg_ret jg_parse_str(
    jg_t * jg,
    char const * json_str,
    size_t byte_c
);

jg_ret jg_parse_str_no_copy(
    jg_t * jg,
    char const * json_str,
    size_t byte_c
);

jg_ret jg_parse_file(
    jg_t * jg,
    char const * filepath
);
