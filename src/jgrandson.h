// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JG_ELEM_C(array) (sizeof(array) / sizeof((array)[0]))
#define JG_MIN(a, b) ((a) < (b) ? (a) : (b))
#define JG_MAX(a, b) ((a) > (b) ? (a) : (b))

// Enforce an opaque pointer API for the following 5 types by keeping
// struct definitions private (jgrandson_internal.h).
typedef struct jgrandson jg_t;
typedef struct jg_arr const jg_arr_get_t;
typedef struct jg_obj const jg_obj_get_t;
typedef struct jg_val_out jg_arr_set_t;
typedef struct jg_val_out jg_obj_set_t;

enum jg_type { // The JSON types
    JG_TYPE_NULL = 0,
    JG_TYPE_BOOL = 1,
    JG_TYPE_NUM = 2,
    JG_TYPE_STR = 3,
    JG_TYPE_ARR = 4,
    JG_TYPE_OBJ = 5
};

// Every possible Jgrandson error condition corresponds to its own unique enum
// return value. Verbose, but unambiguous; plus it allows for a straightforward
// implementation of jg_get_err_str().
typedef enum {
    JG_OK = 0,
    JG_E_STATE_NOT_PARSE = 1,
    JG_E_STATE_NOT_GET = 2,
    JG_E_STATE_NOT_SET = 3,
    JG_E_STATE_NOT_GEN = 4,
    JG_E_MALLOC = 5,
    JG_E_CALLOC = 6,
    JG_E_REALLOC = 7,
    JG_E_NEWLOCALE = 8,
    JG_E_FREAD = 9,
    JG_E_ERRNO_FOPEN = 10,
    JG_E_ERRNO_FCLOSE = 11,
    JG_E_ERRNO_FSEEKO = 12,
    JG_E_ERRNO_FTELLO = 13,
    JG_E_PARSE_INVALID_TYPE = 14,
    JG_E_PARSE_UNTERM_STR = 15,
    JG_E_PARSE_UNTERM_ARR = 16,
    JG_E_PARSE_UNTERM_OBJ = 17,
    JG_E_PARSE_FALSE = 18,
    JG_E_PARSE_TRUE = 19,
    JG_E_PARSE_NULL = 20,
    JG_E_PARSE_NUM_SIGN = 21,
    JG_E_PARSE_NUM_LEAD_ZERO = 22,
    JG_E_PARSE_NUM_INVALID = 23,
    JG_E_PARSE_NUM_MULTIPLE_POINTS = 24,
    JG_E_PARSE_NUM_EXP_HEAD_INVALID = 25,
    JG_E_PARSE_NUM_EXP_INVALID = 26,
    JG_E_PARSE_NUM_TOO_LARGE = 27,
    JG_E_PARSE_STR_UTF8_INVALID = 28,
    JG_E_PARSE_STR_UNESC_CONTROL = 29,
    JG_E_PARSE_STR_ESC_INVALID = 30,
    JG_E_PARSE_STR_UTF16_INVALID = 31,
    JG_E_PARSE_STR_UTF16_UNPAIRED_HIGH = 32,
    JG_E_PARSE_STR_UTF16_UNPAIRED_LOW = 33,
    JG_E_PARSE_STR_TOO_LARGE = 34,
    JG_E_PARSE_ARR_INVALID_SEP = 35,
    JG_E_PARSE_OBJ_INVALID_KEY = 36,
    JG_E_PARSE_OBJ_KEYVAL_INVALID_SEP = 37,
    JG_E_PARSE_OBJ_INVALID_SEP = 38,
    JG_E_PARSE_OBJ_DUPLICATE_KEY = 39,
    JG_E_PARSE_ROOT_SURPLUS = 40,
    JG_E_GET_NOT_NULL = 41,
    JG_E_GET_NOT_BOOL = 42,
    JG_E_GET_NOT_NUM = 43,
    JG_E_GET_NOT_STR = 44,
    JG_E_GET_NOT_ARR = 45,
    JG_E_GET_NOT_OBJ = 46,
    JG_E_GET_ARR_INDEX_OVER = 47,
    JG_E_GET_ARR_TOO_SHORT = 48,
    JG_E_GET_ARR_TOO_LONG = 49,
    JG_E_GET_OBJ_KEY_NOT_FOUND = 50,
    JG_E_GET_OBJ_TOO_SHORT = 51,
    JG_E_GET_OBJ_TOO_LONG = 52,
    JG_E_GET_STR_BYTE_C_TOO_FEW = 53,
    JG_E_GET_STR_BYTE_C_TOO_MANY = 54,
    JG_E_GET_STR_CHAR_C_TOO_FEW = 55,
    JG_E_GET_STR_CHAR_C_TOO_MANY = 56,
    JG_E_GET_NUM_NOT_INTEGER = 57,
    JG_E_GET_NUM_NOT_UNSIGNED = 58,
    JG_E_GET_NUM_SIGNED_TOO_SMALL = 59,
    JG_E_GET_NUM_SIGNED_TOO_LARGE = 60,
    JG_E_GET_NUM_UNSIGNED_TOO_SMALL = 61,
    JG_E_GET_NUM_UNSIGNED_TOO_LARGE = 62,
    JG_E_GET_NUM_NOT_FLO = 63,
    JG_E_GET_NUM_FLOAT_OUT_OF_RANGE = 64,
    JG_E_GET_NUM_DOUBLE_OUT_OF_RANGE = 65,
    JG_E_GET_NUM_LONG_DOUBLE_OUT_OF_RANGE = 66,
    JG_E_SET_ROOT_ALREADY_SET = 67,
    JG_E_SET_NOT_ARR = 68,
    JG_E_SET_NOT_OBJ = 69,
    JG_E_SET_DUPLICATE_KEY = 70
} jg_ret;

#define JG_GUARD(func_call) do { \
    jg_ret ret = (func_call); \
    if (ret != JG_OK) { \
        return ret; \
    } \
} while (0)

//##############################################################################
//## general prototypes (jg_heap.c) ############################################

jg_t * jg_init(
    void
);

void jg_free(
    jg_t * jg
);

void jg_reinit(
    jg_t * jg
);

//##############################################################################
//## general prototypes (jg_util.c) ############################################

char const * jg_get_err_str(
    jg_t * jg,
    char const * err_mark_before,
    char const * err_mark_after
);

//##############################################################################
//## jg_parse_...() prototypes (jg_parse.c) ####################################

// Copy the JSON text string to a malloc-ed char buffer, then parse.
jg_ret jg_parse_str(
    jg_t * jg,
    char const * json_text, // null-terminator not required
    size_t byte_c // excluding null-terminator
);

// Parse the JSON text string without copying it to a malloc-ed char buffer.
jg_ret jg_parse_callerstr(
    jg_t * jg,
    char const * json_text, // null-terminator not required
    size_t byte_c // excluding null-terminator
);

// Open file, copy contents to a malloc-ed char buffer, close file; then parse.
jg_ret jg_parse_file(
    jg_t * jg,
    char const * filepath
);

//##############################################################################
//## jg_[root|arr|obj]_get_...() prototypes (jg_get.c) #########################

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_json_type() ///////////////////////////////////////////

jg_ret jg_root_get_json_type(
    jg_t * jg,
    enum jg_type * type
);

jg_ret jg_arr_get_json_type(
    jg_t * jg,
    jg_arr_get_t * arr,
    size_t arr_i,
    enum jg_type * type
);

jg_ret jg_obj_get_json_type(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    enum jg_type * type
);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_arr() /////////////////////////////////////////////////

struct jg_opt_arr {
    char const * min_c_reason;
    char const * max_c_reason;
    size_t min_c;
    size_t max_c;
};
struct jg_opt_arr_defa {
    char const * max_c_reason;
    size_t max_c;
};

typedef struct jg_opt_arr jg_root_arr;
jg_ret jg_root_get_arr(
    jg_t * jg,
    jg_root_arr * opt,
    jg_arr_get_t * * v,
    size_t * elem_c
);

typedef struct jg_opt_arr jg_arr_arr;
jg_ret jg_arr_get_arr(
    jg_t * jg,
    jg_arr_get_t * arr,
    size_t arr_i,
    jg_arr_arr * opt,
    jg_arr_get_t * * v,
    size_t * elem_c
);

typedef struct jg_opt_arr jg_obj_arr;
jg_ret jg_obj_get_arr(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    jg_obj_arr * opt,
    jg_arr_get_t * * v,
    size_t * elem_c
);

typedef struct jg_opt_arr_defa jg_obj_arr_defa;
jg_ret jg_obj_get_arr_defa(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    jg_obj_arr_defa * opt,
    jg_arr_get_t * * v,
    size_t * elem_c
);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_obj() /////////////////////////////////////////////////

struct jg_opt_obj {
    char * * * keys;
    size_t * key_c;
    char const * min_c_reason;
    char const * max_c_reason;
    size_t min_c;
    size_t max_c;
};
struct jg_opt_obj_defa {
    char * * * keys;
    size_t * key_c;
    char const * max_c_reason;
    size_t max_c;
};

typedef struct jg_opt_obj jg_root_obj;
jg_ret jg_root_get_obj(
    jg_t * jg,
    jg_root_obj * opt,
    jg_obj_get_t * * v
);

typedef struct jg_opt_obj jg_arr_obj;
jg_ret jg_arr_get_obj(
    jg_t * jg,
    jg_arr_get_t * arr,
    size_t arr_i,
    jg_arr_obj * opt,
    jg_obj_get_t * * v
);

typedef struct jg_opt_obj jg_obj_obj;
jg_ret jg_obj_get_obj(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    jg_obj_obj * opt,
    jg_obj_get_t * * v
);

typedef struct jg_opt_obj_defa jg_obj_obj_defa;
jg_ret jg_obj_get_obj_defa(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    jg_obj_obj_defa * opt,
    jg_obj_get_t * * v
);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_(caller)str() /////////////////////////////////////////

// These getters come in two forms with regard to the destination buffer "v".
//
// 1) jg_[root|arr|obj]_get_str():
//     The string is copied into a malloc()ed char buffer "v". Jgrandson will
//     never free() this buffer: doing so is the responsibility of the caller.
//
// 2) jg_[root|arr|obj]_get_callerstr():
//    The string is copied into a caller-supplied buffer "v". To obtain the size
//    needed for this caller-supplied buffer, use the "sprintf(NULL, 0, ...)
//    paradigm": call ..._get_caller_str() with a "v" of NULL and a non-NULL
//    "byte_c" pointer. (Then allocate a "v" of size "byte_c (+ 1)", and call
//    ..._get_callerstr() again with that non-NULL "v".)

#define JG_OPT_STR_COMMON \
    size_t * byte_c; \
    size_t * char_c; \
    char const * min_byte_c_reason; \
    char const * max_byte_c_reason; \
    char const * min_char_c_reason; \
    char const * max_char_c_reason; \
    size_t min_byte_c; \
    size_t max_byte_c; \
    size_t min_char_c; \
    size_t max_char_c; \
    bool nullify_empty_str; \
    bool omit_null_terminator
//  bool unescape; -- todo: unescaping isn't implemented yet ):
struct jg_opt_str {
    JG_OPT_STR_COMMON;
};
struct jg_opt_obj_str {
    char const * defa;
    JG_OPT_STR_COMMON;
};
#undef JG_OPT_STR_COMMON

typedef struct jg_opt_str jg_root_str;
jg_ret jg_root_get_str(
    jg_t * jg,
    jg_root_str * opt,
    char * * v
);

typedef struct jg_opt_str jg_root_callerstr;
jg_ret jg_root_get_callerstr(
    jg_t * jg,
    jg_root_callerstr * opt,
    char * v
);

typedef struct jg_opt_str jg_arr_str;
jg_ret jg_arr_get_str(
    jg_t * jg,
    jg_arr_get_t * arr,
    size_t arr_i,
    jg_arr_str * opt,
    char * * v
);

typedef struct jg_opt_str jg_arr_callerstr;
jg_ret jg_arr_get_callerstr(
    jg_t * jg,
    jg_arr_get_t * arr,
    size_t arr_i,
    jg_arr_callerstr * opt,
    char * v
);

typedef struct jg_opt_obj_str jg_obj_str;
jg_ret jg_obj_get_str(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    jg_obj_str * opt,
    char * * v
);

typedef struct jg_opt_obj_str jg_obj_callerstr;
jg_ret jg_obj_get_callerstr(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    jg_obj_callerstr * opt,
    char * v
);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_null() ////////////////////////////////////////////////

// Having a "v" parameter would be pointless in the case of ..._get_null().
jg_ret jg_root_get_null(
    jg_t * jg
);

jg_ret jg_arr_get_null(
    jg_t * jg,
    jg_arr_get_t * arr,
    size_t arr_i
);

jg_ret jg_obj_get_null(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key
);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_bool() ////////////////////////////////////////////////

jg_ret jg_root_get_bool(
    jg_t * jg,
    bool * v
);

jg_ret jg_arr_get_bool(
    jg_t * jg,
    jg_arr_get_t * arr,
    size_t arr_i,
    bool * v
);

// Take defaults directly as parameter, since bool has no other options to take.
jg_ret jg_obj_get_bool(
    jg_t * jg,
    jg_obj_get_t * obj,
    char const * key,
    bool const * defa,
    bool * v
);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_<integer_type>() //////////////////////////////////////

// There are many integer getter prototypes but they all take the same form, so
// using these  macros reduces the size of this header by a few hundred lines.

#define JG_ROOT_GET_INT(_suf, _type) \
jg_ret jg_root_get##_suf( \
    jg_t * jg, \
    jg_root##_suf * opt, \
    _type * v \
)

#define JG_ARR_GET_INT(_suf, _type) \
jg_ret jg_arr_get##_suf( \
    jg_t * jg, \
    jg_arr_get_t * arr, \
    size_t arr_i, \
    jg_arr##_suf * opt, \
    _type * v \
)

#define JG_OBJ_GET_INT(_suf, _type) \
jg_ret jg_obj_get##_suf( \
    jg_t * jg, \
    jg_obj_get_t * obj, \
    char const * key, \
    jg_obj##_suf * opt, \
    _type * v \
)

#define JG_GET_INT(_suf, _type) \
struct jg_opt##_suf { \
    char const * min_reason; \
    char const * max_reason; \
    _type min; \
    _type max; \
}; \
struct jg_opt_obj##_suf { \
    _type const * defa; \
    char const * min_reason; \
    char const * max_reason; \
    _type min; \
    _type max; \
}; \
\
typedef struct jg_opt##_suf jg_root##_suf; \
JG_ROOT_GET_INT(_suf, _type); \
\
typedef struct jg_opt##_suf jg_arr##_suf; \
JG_ARR_GET_INT(_suf, _type); \
\
typedef struct jg_opt_obj##_suf jg_obj##_suf; \
JG_OBJ_GET_INT(_suf, _type)

JG_GET_INT(_int8, int8_t);
JG_GET_INT(_char, char);
JG_GET_INT(_signed_char, signed char);
JG_GET_INT(_int16, int16_t);
JG_GET_INT(_short, short);
JG_GET_INT(_int32, int32_t);
JG_GET_INT(_int, int);
JG_GET_INT(_int64, int64_t);
JG_GET_INT(_long, long);
JG_GET_INT(_long_long, long long);
JG_GET_INT(_intmax, intmax_t);

JG_GET_INT(_uint8, uint8_t);
JG_GET_INT(_unsigned_char, unsigned char);
JG_GET_INT(_uint16, uint16_t);
JG_GET_INT(_unsigned_short, unsigned short);
JG_GET_INT(_uint32, uint32_t);
JG_GET_INT(_unsigned, unsigned);
JG_GET_INT(_uint64, uint64_t);
JG_GET_INT(_unsigned_long, unsigned long);
JG_GET_INT(_unsigned_long_long, unsigned long long);
JG_GET_INT(_sizet, size_t);
JG_GET_INT(_uintmax, uintmax_t);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_[float|double|long_double]() //////////////////////////

// Floating point getters may change in the future to take a jg_opt struct
// with .min and .max members, along with .rel_diff, .abs_diff and/or .epsilon,
// but I currently don't have the time/need/motivation to implement that.
// If you'd like to see this: pull requests are very welcome!

#define JG_ROOT_GET_FLO(_suf, _type) \
jg_ret jg_root_get##_suf( \
    jg_t * jg, \
    _type * v \
)

#define JG_ARR_GET_FLO(_suf, _type) \
jg_ret jg_arr_get##_suf( \
    jg_t * jg, \
    jg_arr_get_t * arr, \
    size_t arr_i, \
    _type * v \
)

#define JG_OBJ_GET_FLO(_suf, _type) \
jg_ret jg_obj_get##_suf( \
    jg_t * jg, \
    jg_obj_get_t * obj, \
    char const * key, \
    _type const * defa, \
    _type * v \
)

#define JG_GET_FLO(_suf, _type) \
JG_ROOT_GET_FLO(_suf, _type); \
JG_ARR_GET_FLO(_suf, _type); \
JG_ARR_GET_FLO(_suf, _type)

JG_GET_FLO(_float, float);
JG_GET_FLO(_double, double);
JG_GET_FLO(_long_double, long double);

//##############################################################################
//## jg_[root|arr|obj]_set_...() prototypes (jg_set.c) #########################

jg_ret jg_root_set_null(
    jg_t * jg
);

jg_ret jg_arr_set_null(
    jg_t * jg,
    jg_arr_set_t * arr
);

jg_ret jg_obj_set_null(
    jg_t * jg,
    jg_obj_set_t * obj,
    char const * key
);

#define JG_ROOT_SET(_suf, _type) \
jg_ret jg_root_set##_suf( \
    jg_t * jg, \
    _type v \
)

#define JG_ARR_SET(_suf, _type) \
jg_ret jg_arr_set##_suf( \
    jg_t * jg, \
    jg_arr_set_t * arr, \
    _type v \
)

#define JG_OBJ_SET(_suf, _type) \
jg_ret jg_obj_set##_suf( \
    jg_t * jg, \
    jg_obj_set_t * obj, \
    char const * key, \
    _type v \
)

#define JG_SET(_suf, _type) \
JG_ROOT_SET(_suf, _type); \
JG_ARR_SET(_suf, _type); \
JG_OBJ_SET(_suf, _type)

JG_SET(_arr, jg_arr_set_t * *);

JG_SET(_obj, jg_obj_set_t * *);

JG_SET(_str, char const *);
JG_SET(_callerstr, char const *);

JG_SET(_bool, bool);

JG_SET(_int8, int8_t);
JG_SET(_char, char);
JG_SET(_signed_char, signed char);
JG_SET(_int16, int16_t);
JG_SET(_short, short);
JG_SET(_int32, int32_t);
JG_SET(_int, int);
JG_SET(_int64, int64_t);
JG_SET(_long, long);
JG_SET(_long_long, long long);
JG_SET(_intmax, intmax_t);

JG_SET(_uint8, uint8_t);
JG_SET(_unsigned_char, unsigned char);
JG_SET(_uint16, uint16_t);
JG_SET(_unsigned_short, unsigned short);
JG_SET(_uint32, uint32_t);
JG_SET(_unsigned, unsigned);
JG_SET(_uint64, uint64_t);
JG_SET(_unsigned_long, unsigned long);
JG_SET(_unsigned_long_long, unsigned long long);
JG_SET(_sizet, size_t);
JG_SET(_uintmax, uintmax_t);

JG_SET(_float, float);
JG_SET(_double, double);
JG_SET(_long_double, long double);
