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

#define JG_ELEM_C(array) (sizeof(array) / sizeof((array)[0]))
#define JG_MIN(a, b) ((a) < (b) ? (a) : (b))
#define JG_MAX(a, b) ((a) > (b) ? (a) : (b))

#define JG_GUARD(func_call) do { \
    jg_ret ret = (func_call); \
    if (ret != JG_OK) { \
        return ret; \
    } \
} while (0)

typedef struct jgrandson jg_t;
typedef struct jg_val const jg_arr_t;
typedef struct jg_val const jg_obj_t;

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
    JG_E_PARSE_NUM_EXP_HEAD_INVALID = 20,
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
    JG_E_PARSE_OBJ_INVALID_SEP = 31,
    JG_E_PARSE_ROOT_SURPLUS = 32,
    JG_E_WRONG_STATE = 33,
    JG_E_GET_WRONG_SRC_TYPE = 34,
    JG_E_GET_WRONG_DST_TYPE = 35,
    JG_E_GET_SRC_ARR_OVER = 36,
    JG_E_GET_KEY_NOT_FOUND = 37,
    JG_E_GET_ARR_TOO_SHORT = 38,
    JG_E_GET_ARR_TOO_LONG = 39,
    JG_E_GET_STR_BYTE_C_TOO_FEW = 40,
    JG_E_GET_STR_BYTE_C_TOO_MANY = 41,
    JG_E_GET_STR_CHAR_C_TOO_FEW = 42,
    JG_E_GET_STR_CHAR_C_TOO_MANY = 43,
    JG_E_GET_STR_CUSTOM = 44
} jg_ret;

enum jg_type {
    JG_TYPE_NULL = 0,
    JG_TYPE_ARR = 1,
    JG_TYPE_OBJ = 2,
    JG_TYPE_STR = 3,
    JG_TYPE_NUM = 4,
    JG_TYPE_FALSE = 5,
    JG_TYPE_TRUE = 6
};

jg_t * jg_init(
    void
);

void jg_free(
    jg_t * jg
);

char const * jg_get_err_str(
    jg_t * jg,
    char const * parse_err_mark_before,
    char const * parse_err_mark_after
);

////////////////////////////////////////////////////////////////////////////////
// jg_parse_... prototypes /////////////////////////////////////////////////////

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

jg_ret jg_parse_ustr(
    jg_t * jg,
    uint8_t const * json_str,
    size_t byte_c
);

jg_ret jg_parse_ustr_no_copy(
    jg_t * jg,
    uint8_t const * json_str,
    size_t byte_c
);

jg_ret jg_parse_file(
    jg_t * jg,
    char const * filepath
);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_... prototypes ////////////////////////////////////////

// jg_[root|arr|obj]_get_json_type(): 

jg_ret jg_root_get_json_type(jg_t * jg,
    enum jg_type * dst);

jg_ret jg_arr_get_json_type(jg_t * jg, jg_arr_t const * src, size_t src_i,
    enum jg_type * dst);

jg_ret jg_obj_get_json_type(jg_t * jg, jg_obj_t const * src, char const * src_k,
    enum jg_type * dst);

// jg_[root|arr|obj]_get_arr(): 

jg_ret jg_root_get_arr(jg_t * jg,
    size_t min_c, size_t max_c, jg_arr_t * * dst, size_t * elem_c);

jg_ret jg_arr_get_arr(jg_t * jg, jg_arr_t const * src, size_t src_i,
    size_t min_c, size_t max_c, jg_arr_t * * dst, size_t * elem_c);

jg_ret jg_obj_get_arr(jg_t * jg, jg_obj_t const * src, char const * src_k,
    size_t min_c, size_t max_c, jg_arr_t * * dst, size_t * elem_c);

jg_ret jg_obj_get_arr_defa(jg_t * jg, jg_obj_t const * src, char const * src_k,
    size_t max_c, jg_arr_t * * dst, size_t * elem_c);

// jg_[root|arr|obj]_get_obj(): 

jg_ret jg_root_get_obj(jg_t * jg,
    jg_obj_t * * dst);

jg_ret jg_arr_get_obj(jg_t * jg, jg_arr_t const * src, size_t src_i,
    jg_obj_t * * dst);

jg_ret jg_obj_get_obj(jg_t * jg, jg_obj_t const * src, char const * src_k,
    jg_obj_t * * dst);

jg_ret jg_obj_get_obj_defa(jg_t * jg, jg_obj_t const * src, char const * src_k,
    jg_obj_t * * dst);

// jg_obj_get_keys(): 

jg_ret jg_obj_get_keys(jg_t * jg, jg_obj_t * src,
    char * * * dst, size_t * key_c);

// jg_[root|arr|obj]_get_*str*(): 

typedef struct {
    size_t * byte_c;
    size_t * char_c;
    size_t min_byte_c;
    size_t max_byte_c;
    size_t min_char_c;
    size_t max_char_c;
    char const * min_byte_c_estr;
    char const * max_byte_c_estr;
    char const * min_char_c_estr;
    char const * max_char_c_estr;
    bool nullify_empty_str;
    bool omit_null_terminator;
    // bool keep_escaped; -- todo: unescaping (or not) isn't implemented yet ):
} jg_opt_str;

jg_ret jg_root_get_str(jg_t * jg,
    char * dst, jg_opt_str * opt);

jg_ret jg_root_get_stra(jg_t * jg,
    char * * dst, jg_opt_str * opt);

jg_ret jg_root_get_ustr(jg_t * jg,
    uint8_t * dst, jg_opt_str * opt);

jg_ret jg_root_get_ustra(jg_t * jg,
    uint8_t * * dst, jg_opt_str * opt);

jg_ret jg_arr_get_str(jg_t * jg, jg_arr_t const * src, size_t src_i,
    char * dst, jg_opt_str * opt);

jg_ret jg_arr_get_stra(jg_t * jg, jg_arr_t const * src, size_t src_i,
    char * * dst, jg_opt_str * opt);

jg_ret jg_arr_get_ustr(jg_t * jg, jg_arr_t const * src, size_t src_i,
    uint8_t * dst, jg_opt_str * opt);

jg_ret jg_arr_get_ustra(jg_t * jg, jg_arr_t const * src, size_t src_i,
    uint8_t * * dst, jg_opt_str * opt);

jg_ret jg_obj_get_str(jg_t * jg, jg_obj_t const * src, char const * src_k,
    char const * defa, char * dst, jg_opt_str * opt);

jg_ret jg_obj_get_stra(jg_t * jg, jg_obj_t const * src, char const * src_k,
    char const * defa, char * * dst, jg_opt_str * opt);

jg_ret jg_obj_get_ustr(jg_t * jg, jg_obj_t const * src, char const * src_k,
    uint8_t const * defa, uint8_t * dst, jg_opt_str * opt);

jg_ret jg_obj_get_ustra(jg_t * jg, jg_obj_t const * src, char const * src_k,
    uint8_t const * defa, uint8_t * * dst, jg_opt_str * opt);

// todo: all other types

/*
typedef struct {
    intmax_t * min,
    intmax_t * max,
    char const * min_estr,
    char const * max_estr
} jg_opt_int;

typedef struct {
    uintmax_t * min,
    uintmax_t * max,
    char const * min_estr,
    char const * max_estr
} jg_opt_uint;
*/
