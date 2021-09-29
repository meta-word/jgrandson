// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifdef _MSC_VER // If any version of Visual Studio:
#define _CRT_SECURE_NO_WARNINGS // No whining about decent Posix functions!
#pragma warning(disable: 5105) // Microsoft complaining about its own winbase.h!
#pragma warning(disable: 6011) // Also, Microsoft's...
#pragma warning(disable: 6386) // static analysis game...
#pragma warning(disable: 6387) // has a number of weaknesses.
#endif
#define WIN32_LEAN_AND_MEAN // Cut down on the amount of included cruft
#include <windows.h>
#endif

//##############################################################################
//# C API ####### (See the bottom of this header for the inline C++ API) #######
//##############################################################################

#if defined (__cplusplus)
extern "C" {
#endif

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
    JG_E_STATE_NOT_GENERATE = 4,
    JG_E_SET_ROOT_ALREADY_SET = 5,
    JG_E_SET_NOT_ARR = 6,
    JG_E_SET_NOT_OBJ = 7,
    JG_E_SET_OBJ_DUPLICATE_KEY = 8,
    JG_E_MALLOC = 9,
    JG_E_CALLOC = 10,
    JG_E_REALLOC = 11,
    JG_E_VSPRINTF = 12,
    JG_E_VSNPRINTF = 13,
    JG_E_NEWLOCALE = 14,
    JG_E_FREAD = 15,
    JG_E_FWRITE = 16,
    JG_E_ERRNO_FOPEN = 17,
    JG_E_ERRNO_FCLOSE = 18,
    JG_E_ERRNO_FSEEKO = 19,
    JG_E_ERRNO_FTELLO = 20,
    JG_E_PARSE_INVALID_TYPE = 21,
    JG_E_PARSE_UNTERM_STR = 22,
    JG_E_PARSE_UNTERM_ARR = 23,
    JG_E_PARSE_UNTERM_OBJ = 24,
    JG_E_PARSE_FALSE = 25,
    JG_E_PARSE_TRUE = 26,
    JG_E_PARSE_NULL = 27,
    JG_E_PARSE_NUM_SIGN = 28,
    JG_E_PARSE_NUM_LEAD_ZERO = 29,
    JG_E_PARSE_NUM_INVALID = 30,
    JG_E_PARSE_NUM_MULTIPLE_POINTS = 31,
    JG_E_PARSE_NUM_EXP_HEAD_INVALID = 32,
    JG_E_PARSE_NUM_EXP_INVALID = 33,
    JG_E_PARSE_NUM_TOO_LARGE = 34,
    JG_E_PARSE_STR_UTF8_INVALID = 35,
    JG_E_PARSE_STR_UNESC_CONTROL = 36,
    JG_E_PARSE_STR_ESC_INVALID = 37,
    JG_E_PARSE_STR_UTF16_INVALID = 38,
    JG_E_PARSE_STR_UTF16_UNPAIRED_LOW = 39,
    JG_E_PARSE_STR_UTF16_UNPAIRED_HIGH = 40,
    JG_E_PARSE_STR_TOO_LARGE = 41,
    JG_E_PARSE_ARR_INVALID_SEP = 42,
    JG_E_PARSE_OBJ_INVALID_KEY = 43,
    JG_E_PARSE_OBJ_KEYVAL_INVALID_SEP = 44,
    JG_E_PARSE_OBJ_INVALID_SEP = 45,
    JG_E_PARSE_OBJ_DUPLICATE_KEY = 46,
    JG_E_PARSE_ROOT_SURPLUS = 47,
    JG_E_GET_ARG_IS_NULL = 48,
    JG_E_GET_NOT_NULL = 49,
    JG_E_GET_NOT_BOOL = 50,
    JG_E_GET_NOT_NUM = 51,
    JG_E_GET_NOT_STR = 52,
    JG_E_GET_NOT_ARR = 53,
    JG_E_GET_NOT_OBJ = 54,
    JG_E_GET_ARR_INDEX_OVER = 55,
    JG_E_GET_ARR_TOO_SHORT = 56,
    JG_E_GET_ARR_TOO_LONG = 57,
    JG_E_GET_OBJ_KEY_NOT_FOUND = 58,
    JG_E_GET_OBJ_TOO_SHORT = 59,
    JG_E_GET_OBJ_TOO_LONG = 60,
    JG_E_GET_STR_BYTE_C_TOO_FEW = 61,
    JG_E_GET_STR_BYTE_C_TOO_MANY = 62,
    JG_E_GET_STR_CHAR_C_TOO_FEW = 63,
    JG_E_GET_STR_CHAR_C_TOO_MANY = 64,
    JG_E_GET_NUM_NOT_INTEGER = 65,
    JG_E_GET_NUM_NOT_UNSIGNED = 66,
    JG_E_GET_NUM_SIGNED_TOO_SMALL = 67,
    JG_E_GET_NUM_SIGNED_TOO_LARGE = 68,
    JG_E_GET_NUM_UNSIGNED_TOO_SMALL = 69,
    JG_E_GET_NUM_UNSIGNED_TOO_LARGE = 70,
    JG_E_GET_NUM_NOT_FLO = 71,
    JG_E_GET_NUM_FLOAT_OUT_OF_RANGE = 72,
    JG_E_GET_NUM_DOUBLE_OUT_OF_RANGE = 73,
    JG_E_GET_NUM_LONG_DOUBLE_OUT_OF_RANGE = 74
} jg_ret;

#define JG_GUARD(func_call) do { \
    jg_ret ret = (func_call); \
    if (ret != JG_OK) { \
        return ret; \
    } \
} while (0)

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
// There's a long-standing bug in GCC where it mistakenly emits a
// -Wmissing-field-initializers in cases where fields are initialized with
// designated initializers: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84685
// This is annoying given that designated field initializers are a key
// ingredient to Jgrandson's flexible optional argument passing.
// As a workaround, suppress such noise for any file that includes this header:
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

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
//## general prototypes (jg_error.c) ###########################################

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

// All remaining getters have the same form of protottpe. Use of the following
// macros macros reduces the size of this header by a few hundred lines.

#define JG_ROOT_GET(_suf, _type) \
jg_ret jg_root_get##_suf( \
    jg_t * jg, \
    jg_root##_suf * opt, \
    _type * v \
)

#define JG_ARR_GET(_suf, _type) \
jg_ret jg_arr_get##_suf( \
    jg_t * jg, \
    jg_arr_get_t * arr, \
    size_t arr_i, \
    jg_arr##_suf * opt, \
    _type * v \
)

#define JG_OBJ_GET(_suf, _type) \
jg_ret jg_obj_get##_suf( \
    jg_t * jg, \
    jg_obj_get_t * obj, \
    char const * key, \
    jg_obj##_suf * opt, \
    _type * v \
)

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
JG_ROOT_GET(_obj, jg_obj_get_t *);

typedef struct jg_opt_obj jg_arr_obj;
JG_ARR_GET(_obj, jg_obj_get_t *);

typedef struct jg_opt_obj jg_obj_obj;
JG_OBJ_GET(_obj, jg_obj_get_t *);

typedef struct jg_opt_obj_defa jg_obj_obj_defa;
JG_OBJ_GET(_obj_defa, jg_obj_get_t *);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_(json)_(caller)str() //////////////////////////////////

// These getters come in 2 * 2 = 4 forms:
// 
// * 2 forms with regard to escape sequences (see also rfc8259):
//
// 1) jg_[root|arr|obj]_get_(caller)str():
//    All escape sequences as they may appear in the JSON string in the JSON
//    text are unescaped prior to copying their contents to the destination;
//    except for any escaped null-terminators ("\u0000"), which are discarded.
//
// 2) jg_[root|arr|obj]_get_json_(caller)str():
//    All escape sequence contained in the JSON string are left unescaped: the
//    JSON string is copied to the destination as-is.
//
// * And 2 forms with regard to the destination buffer "v":
//
// 1) jg_[root|arr|obj]_get_(json)_str():
//    The string is copied into a malloc()ed char buffer "v". Jgrandson will
//    never free() this buffer: doing so is the responsibility of the caller.
//
// 2) jg_[root|arr|obj]_get_(json)_callerstr():
//    The string is copied into a caller-supplied buffer "v". To obtain the size
//    needed for this caller-supplied buffer, use the "sprintf(NULL, 0, ...)
//    paradigm": call ..._get_caller_str() with a "v" of NULL and a non-NULL
//    "byte_c" pointer. (Then allocate a "v" of size "byte_c (+ 1)", and call
//    ..._get_callerstr() again with that non-NULL "v".)

#define JG_OPT_STR_COMMON \
    size_t * byte_c; \
    size_t * codepoint_c; \
    char const * min_byte_c_reason; \
    char const * max_byte_c_reason; \
    char const * min_cp_c_reason; \
    char const * max_cp_c_reason; \
    size_t min_byte_c; \
    size_t max_byte_c; \
    size_t min_cp_c; \
    size_t max_cp_c; \
    bool nullify_empty_str; \
    bool omit_null_terminator

struct jg_opt_str {
    JG_OPT_STR_COMMON;
};

struct jg_opt_obj_str {
    char const * defa;
    JG_OPT_STR_COMMON;
};

#undef JG_OPT_STR_COMMON

typedef struct jg_opt_str jg_root_str;
JG_ROOT_GET(_str, char *);

typedef struct jg_opt_str jg_root_callerstr;
JG_ROOT_GET(_callerstr, char);

typedef struct jg_opt_str jg_root_json_str;
JG_ROOT_GET(_json_str, char *);

typedef struct jg_opt_str jg_root_json_callerstr;
JG_ROOT_GET(_json_callerstr, char);


typedef struct jg_opt_str jg_arr_str;
JG_ARR_GET(_str, char *);

typedef struct jg_opt_str jg_arr_callerstr;
JG_ARR_GET(_callerstr, char);

typedef struct jg_opt_str jg_arr_json_str;
JG_ARR_GET(_json_str, char *);

typedef struct jg_opt_str jg_arr_json_callerstr;
JG_ARR_GET(_json_callerstr, char);


typedef struct jg_opt_obj_str jg_obj_str;
JG_OBJ_GET(_str, char *);

typedef struct jg_opt_obj_str jg_obj_callerstr;
JG_OBJ_GET(_callerstr, char);

typedef struct jg_opt_obj_str jg_obj_json_str;
JG_OBJ_GET(_json_str, char *);

typedef struct jg_opt_obj_str jg_obj_json_callerstr;
JG_OBJ_GET(_json_callerstr, char);

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_<integer_type>() //////////////////////////////////////

#define JG_GET_INT(_suf, _type) \
struct jg_opt##_suf { \
    char const * min_reason; \
    char const * max_reason; \
    _type * min; \
    _type * max; \
}; \
\
struct jg_opt_obj##_suf { \
    _type const * defa; \
    char const * min_reason; \
    char const * max_reason; \
    _type * min; \
    _type * max; \
}; \
\
typedef struct jg_opt##_suf jg_root##_suf; \
JG_ROOT_GET(_suf, _type); \
\
typedef struct jg_opt##_suf jg_arr##_suf; \
JG_ARR_GET(_suf, _type); \
\
typedef struct jg_opt_obj##_suf jg_obj##_suf; \
JG_OBJ_GET(_suf, _type)

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
JG_OBJ_GET_FLO(_suf, _type)

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

//##############################################################################
//## jg_generate_...() prototypes (jg_generate.c) ##############################

struct jg_opt_whitespace {
    // The number of spaces (or tabs if indent_is_tab is true) to use for each
    // indentation level. Ignored if no_whitespace is true.
    size_t * indent; // Default: 2

    // If true, use tab characters instead of spaces for indentation.
    bool indent_is_tab; // Default: false

    // If true, prepend a carriage return ('\r') to any line feed ('\n'), for
    // Windows-style ("\r\n") newlines instead of Unix-style ("\n") newlines.
    bool include_cr; // Default: false

    // If true, don't bother including any whitespace at all (and ignore the
    // options above). Does not affect the .no_newline_before_eof flag below.
    bool no_whitespace; // Default: false

    // Only applicable to jg_generate_file(): by default Jgrandson writes a
    // final newline when saving the generated JSON as a text file -- as
    // is expected of all text files in the Unix world. Set this to true to
    // overrule that default.
    bool no_newline_before_eof; // jg_generate_file() default: false
};

typedef struct jg_opt_whitespace jg_opt_whitespace;

jg_ret jg_generate_str(
    jg_t * jg,
    jg_opt_whitespace * opt,
    char * * json_text,
    size_t * byte_c
);

jg_ret jg_generate_callerstr(
    jg_t * jg,
    jg_opt_whitespace * opt,
    char * json_text,
    size_t * byte_c
);

jg_ret jg_generate_file(
    jg_t * jg,
    jg_opt_whitespace * opt,
    char const * filepath
);

#if defined (__cplusplus)
} // End of extern "C"

//##############################################################################
//# C++ API ####################################################################
//##############################################################################

#include <concepts>
#include <filesystem>
#include <functional>
#include <fstream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::placeholders;

#ifdef _MSC_VER // If any version of Visual Studio
#pragma warning(disable: 26812) // Ignore complaints about unscoped enum jg_ret
#endif

namespace jg {

struct Err : public std::runtime_error {
    Err(std::string const & what = std::string()) : std::runtime_error(what) {}
};
class ErrMemory   : public Err { using Err::Err; };
class ErrState    : public Err { using Err::Err; };
class ErrFile     : public Err { using Err::Err; };
class ErrParse    : public Err { using Err::Err; };
class ErrArg      : public Err { using Err::Err; };
class ErrSet      : public Err { using Err::Err; };
class ErrGetRange : public Err { using Err::Err; };
class ErrGetKey   : public Err { using Err::Err; };
class ErrGetType  : public Err { using Err::Err; };
class ErrGetStr   : public Err { using Err::Err; };
class ErrGetNum   : public Err { using Err::Err; };

struct _Session {
    inline _Session() {
        jg = jg_init();
        if (!jg) {
            throw ErrMemory(
                "jg_init() returned a NULL pointer: out of memory?");
        }
    }

    inline ~_Session() noexcept {
        if (jg) {
            jg_free(jg);
        }
    }

    jg_t * jg{};
};

class Base {
protected:
    void guard(jg_ret ret) const {
        if (ret == JG_OK) {
            return;
        }
        auto str = jg_get_err_str(_s->jg, nullptr, nullptr);
        switch (ret) {
        case JG_E_STATE_NOT_PARSE: case JG_E_STATE_NOT_GET:
        case JG_E_STATE_NOT_SET: case JG_E_STATE_NOT_GENERATE:
            throw ErrState(str);
        case JG_E_SET_ROOT_ALREADY_SET: case JG_E_SET_NOT_ARR:
        case JG_E_SET_NOT_OBJ: case JG_E_SET_OBJ_DUPLICATE_KEY:
            throw ErrSet(str);
        case JG_E_MALLOC: case JG_E_CALLOC: case JG_E_REALLOC:
        case JG_E_VSPRINTF: case JG_E_VSNPRINTF: case JG_E_NEWLOCALE:
            throw ErrMemory(str);
        case JG_E_FREAD: case JG_E_FWRITE: case JG_E_ERRNO_FOPEN:
        case JG_E_ERRNO_FCLOSE: case JG_E_ERRNO_FSEEKO: case JG_E_ERRNO_FTELLO:
            throw ErrFile(str);
        case JG_E_PARSE_INVALID_TYPE: case JG_E_PARSE_UNTERM_STR:
        case JG_E_PARSE_UNTERM_ARR: case JG_E_PARSE_UNTERM_OBJ:
        case JG_E_PARSE_FALSE: case JG_E_PARSE_TRUE: case JG_E_PARSE_NULL:
        case JG_E_PARSE_NUM_SIGN: case JG_E_PARSE_NUM_LEAD_ZERO:
        case JG_E_PARSE_NUM_INVALID: case JG_E_PARSE_NUM_MULTIPLE_POINTS:
        case JG_E_PARSE_NUM_EXP_HEAD_INVALID: case JG_E_PARSE_NUM_EXP_INVALID:
        case JG_E_PARSE_NUM_TOO_LARGE: case JG_E_PARSE_STR_UTF8_INVALID:
        case JG_E_PARSE_STR_UNESC_CONTROL: case JG_E_PARSE_STR_ESC_INVALID:
        case JG_E_PARSE_STR_UTF16_INVALID:
        case JG_E_PARSE_STR_UTF16_UNPAIRED_LOW:
        case JG_E_PARSE_STR_UTF16_UNPAIRED_HIGH: case JG_E_PARSE_STR_TOO_LARGE:
        case JG_E_PARSE_ARR_INVALID_SEP: case JG_E_PARSE_OBJ_INVALID_KEY:
        case JG_E_PARSE_OBJ_KEYVAL_INVALID_SEP: case JG_E_PARSE_OBJ_INVALID_SEP:
        case JG_E_PARSE_OBJ_DUPLICATE_KEY: case JG_E_PARSE_ROOT_SURPLUS:
            throw ErrParse(str);
        case JG_E_GET_ARG_IS_NULL:
            throw ErrArg(str);
        case JG_E_GET_NOT_NULL: case JG_E_GET_NOT_BOOL: case JG_E_GET_NOT_NUM:
        case JG_E_GET_NOT_STR: case JG_E_GET_NOT_ARR: case JG_E_GET_NOT_OBJ:
            throw ErrGetType(str);
        case JG_E_GET_ARR_INDEX_OVER: case JG_E_GET_ARR_TOO_SHORT:
        case JG_E_GET_ARR_TOO_LONG: case JG_E_GET_OBJ_TOO_SHORT:
        case JG_E_GET_OBJ_TOO_LONG:
            throw ErrGetRange(str);
        case JG_E_GET_OBJ_KEY_NOT_FOUND:
            throw ErrGetKey(str);
        case JG_E_GET_STR_BYTE_C_TOO_FEW: case JG_E_GET_STR_BYTE_C_TOO_MANY:
        case JG_E_GET_STR_CHAR_C_TOO_FEW: case JG_E_GET_STR_CHAR_C_TOO_MANY:
             throw ErrGetStr(str);
        case JG_E_GET_NUM_NOT_INTEGER: case JG_E_GET_NUM_NOT_UNSIGNED:
        case JG_E_GET_NUM_SIGNED_TOO_SMALL: case JG_E_GET_NUM_SIGNED_TOO_LARGE:
        case JG_E_GET_NUM_UNSIGNED_TOO_SMALL:
        case JG_E_GET_NUM_UNSIGNED_TOO_LARGE: case JG_E_GET_NUM_NOT_FLO:
        case JG_E_GET_NUM_FLOAT_OUT_OF_RANGE:
        case JG_E_GET_NUM_DOUBLE_OUT_OF_RANGE:
        case JG_E_GET_NUM_LONG_DOUBLE_OUT_OF_RANGE:
            throw ErrGetNum(str);
        default:
            throw Err("Unrecognized jg_ret value " + std::to_string(ret) +
                ": " + str);
        }
    }

    template <typename Functor> inline auto get_arr(
        Functor functor,
        size_t min_c = 0,
        size_t max_c = 0,
        std::string const & min_c_reason = std::string(),
        std::string const & max_c_reason = std::string()
    ) const;

    template <typename Functor> inline auto get_obj(
        Functor functor,
        size_t min_c = 0,
        size_t max_c = 0,
        std::string const & min_c_reason = std::string(),
        std::string const & max_c_reason = std::string()
    ) const;

    template <typename Functor, typename Opt> inline auto get_str(
        Functor functor,
        Opt && opt,
        size_t min_byte_c = 0,
        size_t max_byte_c = 0,
        size_t min_cp_c = 0,
        size_t max_cp_c = 0,
        // Cannot use std::string_view: null-terminated .c_str() needed
        std::string const & min_byte_c_reason = std::string(),
        std::string const & max_byte_c_reason = std::string(),
        std::string const & min_cp_c_reason = std::string(),
        std::string const & max_cp_c_reason = std::string(),
        size_t * codepoint_c = nullptr
    ) const {
        size_t byte_c{};

        opt.byte_c = &byte_c;
        opt.codepoint_c = codepoint_c;
        opt.min_byte_c_reason =
            min_byte_c_reason.empty() ? nullptr : min_byte_c_reason.c_str();
        opt.max_byte_c_reason =
            max_byte_c_reason.empty() ? nullptr : max_byte_c_reason.c_str();
        opt.min_cp_c_reason =
            min_cp_c_reason.empty() ? nullptr : min_cp_c_reason.c_str();
        opt.max_cp_c_reason =
            max_cp_c_reason.empty() ? nullptr : max_cp_c_reason.c_str();
        opt.min_byte_c = min_byte_c;
        opt.max_byte_c = max_byte_c;
        opt.min_cp_c = min_cp_c;
        opt.max_cp_c = max_cp_c;
        opt.omit_null_terminator = true;

        guard(functor(&opt, nullptr));
        std::string str(byte_c, ' ');
        guard(functor(&opt, &str[0]));
        return str;
    }

    std::shared_ptr<jg::_Session const> _s{};
};

#undef _

#define _JGP_ARGS(...) __VA_ARGS__
#define _JGP_ARGS_MINMAX_NDEFA(_type, ...) __VA_ARGS__
#define _JGP_ARGS_MINMAX_DEFA(_type, ...) _type defa, __VA_ARGS__
#define _JGP_ARGS_MINMAX(_args, _type, _type_min, _type_max) \
    _##_args(_type, \
        _type min = _type_min, \
        _type max = _type_max, \
        /* Cannot use std::string_view: null-terminated .c_str() needed */ \
        std::string const & min_reason = std::string(), \
        std::string const & max_reason = std::string() \
    )

#define _JGP_OPT
#define _JGP_OPT_MINMAX_NDEFA(_opt_t, _defa_ptr, ...) \
    _opt_t opt { __VA_ARGS__ }
#define _JGP_OPT_MINMAX_DEFA(_opt_t, _defa_ptr, ...) \
    _opt_t opt { _defa_ptr, __VA_ARGS__ }
#define _JGP_OPT_MINMAX(_opt, _opt_t, _defa_ptr, _type, _type_min, _type_max) \
    _##_opt(_opt_t, _defa_ptr, \
        min_reason.empty() ? nullptr : min_reason.c_str(), \
        max_reason.empty() ? nullptr : max_reason.c_str(), \
        min == _type_min ? nullptr : &min, \
        max == _type_max ? nullptr : &max \
    )

#define JGP_GET(_suf, _type, _args, _opt, _func) \
    inline auto get##_suf(_##_args) const { \
        _##_opt; \
        _type v{}; \
        guard(_func); \
        return v; \
    }

#define _JGP_GET_ROOT(_suf, _type) \
    JGP_GET(_suf, _type, JGP_ARGS(), JGP_OPT, \
        jg_root_get##_suf(_s->jg, &v) \
    )
#define _JGP_GET_ARR(_suf, _type) \
    JGP_GET(_suf, _type, JGP_ARGS(), JGP_OPT, \
        jg_arr_get##_suf(_s->jg, _arr, _i, &v) \
    )
#define _JGP_GET_OBJ(_suf, _type) \
    JGP_GET(_suf, _type, JGP_ARGS(), JGP_OPT, \
        jg_obj_get##_suf(_s->jg, _obj, _k, &v) \
    )
#define _JGP_GET_OBJ_NDEFA(_suf, _type) \
    JGP_GET(_suf, _type, JGP_ARGS(), JGP_OPT, \
        jg_obj_get##_suf(_s->jg, _obj, _k, nullptr, &v) \
    )
#define _JGP_GET_OBJ_DEFA(_suf, _type) \
    JGP_GET(_suf##_defa, _type, JGP_ARGS(_type defa), JGP_OPT, \
        jg_obj_get##_suf(_s->jg, _obj, _k, &defa, &v) \
    )
#define _JGP_GET_ROOT_MINMAX(_suf, _type, _type_min, _type_max) \
    JGP_GET(_suf, _type, \
        JGP_ARGS_MINMAX(JGP_ARGS_MINMAX_NDEFA, _type, _type_min, _type_max), \
        JGP_OPT_MINMAX(JGP_OPT_MINMAX_NDEFA, jg_opt##_suf, nullptr, _type, \
            _type_min, _type_max), \
        jg_root_get##_suf(_s->jg, &opt, &v) \
    )
#define _JGP_GET_ARR_MINMAX(_suf, _type, _type_min, _type_max) \
    JGP_GET(_suf, _type, \
        JGP_ARGS_MINMAX(JGP_ARGS_MINMAX_NDEFA, _type, _type_min, _type_max), \
        JGP_OPT_MINMAX(JGP_OPT_MINMAX_NDEFA, jg_opt##_suf, nullptr, _type, \
            _type_min, _type_max), \
        jg_arr_get##_suf(_s->jg, _arr, _i, &opt, &v) \
    )
#define _JGP_GET_OBJ_MINMAX_NDEFA(_suf, _type, _type_min, _type_max) \
    JGP_GET(_suf, _type, \
        JGP_ARGS_MINMAX(JGP_ARGS_MINMAX_NDEFA, _type, _type_min, _type_max), \
        JGP_OPT_MINMAX(JGP_OPT_MINMAX_DEFA, jg_opt_obj##_suf, nullptr, _type, \
            _type_min, _type_max), \
        jg_obj_get##_suf(_s->jg, _obj, _k, &opt, &v) \
    )
#define _JGP_GET_OBJ_MINMAX_DEFA(_suf, _type, _type_min, _type_max) \
    JGP_GET(_suf##_defa, _type, \
        JGP_ARGS_MINMAX(JGP_ARGS_MINMAX_DEFA, _type, _type_min, _type_max), \
        JGP_OPT_MINMAX(JGP_OPT_MINMAX_DEFA, jg_opt_obj##_suf, &defa, _type, \
            _type_min, _type_max), \
        jg_obj_get##_suf(_s->jg, _obj, _k, &opt, &v) \
    )

#define JGP_GET_JSON_TYPE(_get) _##_get(_json_type, jg_type)
#define JGP_GET_BOOL(_get) _##_get(_bool, bool)
#define JGP_GET_INT(_get_minmax) \
    _##_get_minmax(_int8, int8_t, INT8_MIN, INT8_MAX); \
    _##_get_minmax(_char, char, CHAR_MIN, CHAR_MAX); \
    _##_get_minmax(_signed_char, signed char, INT8_MIN, INT8_MAX); \
    _##_get_minmax(_int16, int16_t, INT16_MIN, INT16_MAX); \
    _##_get_minmax(_short, short, SHRT_MIN, SHRT_MAX); \
    _##_get_minmax(_int32, int32_t, INT32_MIN, INT32_MAX); \
    _##_get_minmax(_int, int, INT_MIN, INT_MAX); \
    _##_get_minmax(_int64, int64_t, INT64_MIN, INT64_MAX); \
    _##_get_minmax(_long, long, LONG_MIN, LONG_MAX); \
    _##_get_minmax(_long_long, long long, LLONG_MIN, LLONG_MAX); \
    _##_get_minmax(_intmax, intmax_t, INTMAX_MIN, INTMAX_MAX); \
    _##_get_minmax(_uint8, uint8_t, 0, UINT8_MAX); \
    _##_get_minmax(_unsigned_char, unsigned char, 0, UINT8_MAX); \
    _##_get_minmax(_uint16, uint16_t, 0, UINT16_MAX); \
    _##_get_minmax(_unsigned_short, unsigned short, 0, USHRT_MAX); \
    _##_get_minmax(_uint32, uint32_t, 0, UINT32_MAX); \
    _##_get_minmax(_unsigned, unsigned, 0, UINT_MAX); \
    _##_get_minmax(_uint64, uint64_t, 0, UINT64_MAX); \
    _##_get_minmax(_unsigned_long, unsigned long, 0, LONG_MAX); \
    _##_get_minmax(_unsigned_long_long, unsigned long long, 0, ULLONG_MAX); \
    _##_get_minmax(_sizet, size_t, 0, SIZE_MAX); \
    _##_get_minmax(_uintmax, uintmax_t, 0, UINTMAX_MAX)
#define JGP_GET_FLO(_get) \
    _##_get(_float, float); \
    _##_get(_double, double); \
    _##_get(_long_double, long double)

struct Root : protected Base {
    inline Root() {
        _s = std::make_shared<jg::_Session const>();
    }

    inline void parse_str(
        std::string const & json_text
    ) const {
        guard(jg_parse_str(_s->jg, json_text.c_str(), json_text.size()));
    }

    template <typename Type>
    inline void parse_str(
        std::span<Type> const & json_text
    ) const {
       guard(jg_parse_str(_s->jg, reinterpret_cast<char const *>(&json_text[0]),
            json_text.size_bytes()));
    }

    template <typename Type>
    inline void parse_str(
        std::vector<Type> const & json_text
    ) const {
        guard(parse_str(std::span(json_text)));
    }

    // This path string argument is expected to be UTF-8 encoded. (If it isn't,
    // it's probably more convenient to create a std::filesystem::path object,
    // and use the corresponding overloaded method below.)
    inline void parse_file(
        std::string const & filepath
    ) const {
        guard(jg_parse_file(_s->jg, filepath.c_str()));
    }

    // Explicitly require (C++20) std::filesystem::path for this overload to
    // prevent the compiler from trying to promote std::string, and then getting
    // confused that there already is another overload above taking std::string.
    template <typename Type> requires std::same_as<Type, std::filesystem::path>
    inline void parse_file(
        Type const & filepath
    ) const {
        // Use std::ifstream instead of jg_parse_file() here to automatically
        // handle std::filesystem::path variables encoded in something other
        // than UTF-8. In other words, deal with Microsoft's UTF-16 shenanigans.
        size_t byte_c{};
        try {
            byte_c = std::filesystem::file_size(filepath);
        } catch (std::filesystem::filesystem_error & e) {
            throw ErrFile(
                std::string("Failed to obtain JSON file size: ") + e.what());
        }
        std::vector<char> json_text(byte_c);
        std::ifstream ifs(filepath, std::ios::binary);
        if (!ifs) {
            throw ErrFile("Failed to read JSON file via std::ifstream.");
        }
        ifs.read(&json_text[0], byte_c);
        if (ifs.gcount() != byte_c) {
            throw ErrFile("Number of bytes read via std::ifstream does not "
                "match the file size reported by std::filesystem::file_size()."
            );
        }
        parse_str(json_text);
    }

    template <typename... Args> inline auto get_arr(Args... args) const {
        return Base::get_arr(std::bind(jg_root_get_arr, _s->jg, _1, _2, _3),
            args...);
    }

    template <typename... Args> inline auto get_obj(Args... args) const {
        return Base::get_obj(std::bind(jg_root_get_obj, _s->jg, _1, _2),
            args...);
    }

    template <typename... Args> inline auto get_str(Args... args) const {
        return Base::get_str(std::bind(jg_root_get_callerstr, _s->jg, _1, _2),
            jg_opt_str{}, args...);
    }
    template <typename... Args> inline std::u8string get_u8str(Args... args)
        const {
        auto str = get_str(args...);
        return *reinterpret_cast<std::u8string *>(&str);
    }
    template <typename... Args> inline auto get_json_str(Args... args) const {
        return Base::get_str(std::bind(jg_root_get_json_callerstr, _s->jg, _1,
            _2), jg_opt_str{}, args...);
    }
    template <typename... Args> inline std::u8string get_json_u8str(
        Args... args) const {
        auto json_str = get_json_str(args...);
        return *reinterpret_cast<std::u8string *>(&json_str);
    }

    inline void get_null() const { guard(jg_root_get_null(_s->jg)); }
    JGP_GET_JSON_TYPE(JGP_GET_ROOT);
    JGP_GET_BOOL(JGP_GET_ROOT);
    JGP_GET_INT(JGP_GET_ROOT_MINMAX);
    JGP_GET_FLO(JGP_GET_ROOT);

};

class Container : protected Base {
public:
    inline size_t size() const noexcept { return _elem_c; }
    inline size_t index() const noexcept { return _i; }
    inline bool operator != (Container & rhs) const noexcept {
        return _i < rhs._elem_c;
    }
protected:
    inline Container(
        std::shared_ptr<jg::_Session const> s
    ) noexcept {
        _s = s; // Increment s.use_count
    }

    size_t _elem_c{};
    size_t _i{};
};

class ArrGet : public Container {
public:
    inline ArrGet(
        std::shared_ptr<jg::_Session const> s,
        jg_arr_get_t * arr,
        size_t elem_c
    ) noexcept : Container(s) {
        _elem_c = elem_c;
        _arr = arr;
    }

    inline auto & begin() const noexcept { return *this; }
    inline auto end() const noexcept { return *this; }
    inline auto & operator * () const noexcept { return *this; }
    inline auto & operator ++ () noexcept {
        _i++;
        return *this;
    }
    inline auto operator [] (size_t index) {
        if (index >= _elem_c) {
            throw ErrGetRange("Array received a subscription index beyond its "
                "valid range.");
        }
        _i = index;
        return *this;
    }

    template <typename... Args> inline auto get_arr(Args... args) const {
        return Base::get_arr(std::bind(jg_arr_get_arr, _s->jg, _arr, _i, _1, _2,
            _3), args...);
    }

    template <typename... Args> inline auto get_obj(Args... args) const {
        return Base::get_obj(std::bind(jg_arr_get_obj, _s->jg, _arr, _i, _1,
            _2), args...);
    }

    template <typename... Args> inline auto get_str(Args... args) const {
        return Base::get_str(std::bind(jg_arr_get_callerstr, _s->jg, _arr, _i,
            _1, _2), jg_opt_str{}, args...);
    }
    template <typename... Args> inline std::u8string get_u8str(Args... args)
        const {
        auto str = get_str(args...);
        return *reinterpret_cast<std::u8string *>(&str);
    }
    template <typename... Args> inline auto get_json_str(Args... args) const {
        return Base::get_str(std::bind(jg_arr_get_json_callerstr, _s->jg, _arr,
            _i, _1, _2), jg_opt_str{}, args...);
    }
    template <typename... Args> inline std::u8string get_json_u8str(
        Args... args) const {
        auto json_str = get_json_str(args...);
        return *reinterpret_cast<std::u8string *>(&json_str);
    }

    inline void get_null() const { guard(jg_arr_get_null(_s->jg, _arr, _i)); }
    JGP_GET_JSON_TYPE(JGP_GET_ARR);
    JGP_GET_BOOL(JGP_GET_ARR);
    JGP_GET_INT(JGP_GET_ARR_MINMAX);
    JGP_GET_FLO(JGP_GET_ARR);

private:
    jg_arr_get_t * _arr{};
};

class ObjGet : public Container {
public:
    inline ObjGet(
        std::shared_ptr<jg::_Session const> s,
        jg_obj_get_t * obj,
        char * * keys,
        size_t key_c
    ) noexcept : Container(s) {
        _elem_c = key_c;
        _obj = obj;
        _keys = keys;
    }
    inline ~ObjGet() noexcept {
        // These keys were received from a C API ..._get_obj...() option struct,
        // which hands off heap ownership responsibility to the caller.
        free(_keys);
    }

    inline auto & begin() const noexcept { return *this; }
    inline auto end() const noexcept { return *this; }
    inline auto & operator * () const noexcept { return *this; }
    inline auto & operator ++ () noexcept {
        ++_i < _elem_c ? _k = _keys[_i] : _k = _keys[_elem_c - 1];
        return *this;
    }
    inline auto operator [] (size_t index) {
        if (index >= _elem_c) {
            throw ErrGetRange("Array received a subscription index beyond its "
                "valid range.");
        }
        _k = _keys[_i = index];
        return *this;
    }
    auto & operator [] (std::string const & key) {
        for (_i = 0; _i < _elem_c; _i++) {
            _k = _keys[_i];
            if (_k == key) {
                return *this;
            }
        }
        throw ErrGetKey("Key \"" + key + "\" not found.");
    }

    inline std::string get_key() const noexcept { return _k; }
    inline auto get_keys() const {
        std::vector<std::string> keys(_elem_c);
        auto k = _keys;
        for (auto & key : keys) {
            key = *k++;
        }
        return keys;
    }

    template <typename... Args> inline auto get_arr(Args... args) const {
        return Base::get_arr(std::bind(jg_obj_get_arr, _s->jg, _obj, _k, _1, _2,
            _3), args...);
    }
    inline auto get_arr_defa(
        size_t max_c = 0,
        std::string const & max_c_reason = std::string()
    ) const {
        jg_obj_arr_defa opt{
            max_c_reason.empty() ? nullptr : max_c_reason.c_str(),
            max_c
        };
        jg_arr_get_t * arr{};
        size_t elem_c{};
        guard(jg_obj_get_arr_defa(_s->jg, _obj, _k, &opt, &arr, &elem_c));
        return ArrGet(_s, arr, elem_c);
    }

    template <typename... Args> inline auto get_obj(Args... args) const {
        return Base::get_obj(std::bind(jg_obj_get_obj, _s->jg, _obj, _k, _1,
            _2), args...);
    }
    inline auto get_obj_defa(
        size_t max_c = 0,
        std::string const & max_c_reason = std::string()
    ) const {
        char * * keys{};
        size_t key_c{};
        jg_obj_obj_defa opt{
            &keys,
            &key_c,
            max_c_reason.empty() ? nullptr : max_c_reason.c_str(),
            max_c,
        };
        jg_obj_get_t * obj{};
        guard(jg_obj_get_obj_defa(_s->jg, _obj, _k, &opt, &obj));
        return ObjGet(_s, obj, keys, key_c);
    }

    template <typename... Args> inline auto get_str(Args... args) const {
        return Base::get_str(std::bind(jg_obj_get_callerstr, _s->jg, _obj, _k,
            _1, _2), jg_opt_obj_str{}, args...);
    }
    template <typename... Args> inline std::u8string get_u8str(Args... args)
        const {
        auto str = get_str(args...);
        return *reinterpret_cast<std::u8string *>(&str);
    }
    template <typename... Args> inline auto get_str_defa(
        std::string const & defa, Args... args) const {
        return Base::get_str(std::bind(jg_obj_get_callerstr, _s->jg, _obj, _k,
            _1, _2), jg_opt_obj_str{defa.c_str()}, args...);
    }
    template <typename... Args> inline std::u8string get_u8str_defa(
        Args... args) const {
        auto str_defa = get_str_defa(args...);
        return *reinterpret_cast<std::u8string *>(&str_defa);
    }
    template <typename... Args> inline auto get_json_str(Args... args) const {
        return Base::get_str(std::bind(jg_obj_get_json_callerstr, _s->jg, _obj,
            _k, _1, _2), jg_opt_obj_str{}, args...);
    }
    template <typename... Args> inline std::u8string get_json_u8str(
        Args... args) const {
        auto json_str = get_json_str(args...);
        return *reinterpret_cast<std::u8string *>(&json_str);
    }
    template <typename... Args> inline auto get_json_str_defa(
        std::string const & defa, Args... args) const {
        return Base::get_str(std::bind(jg_obj_get_json_callerstr, _s->jg, _obj,
            _k, _1, _2), jg_opt_obj_str{defa.c_str()}, args...);
    }
    template <typename... Args> inline std::u8string get_json_u8str_defa(
        Args... args) const {
        auto json_str_defa = get_json_str_defa(args...);
        return *reinterpret_cast<std::u8string *>(&json_str_defa);
    }

    inline void get_null() const { guard(jg_obj_get_null(_s->jg, _obj,  _k)); }
    JGP_GET_JSON_TYPE(JGP_GET_OBJ);
    JGP_GET_BOOL(JGP_GET_OBJ_NDEFA);
    JGP_GET_BOOL(JGP_GET_OBJ_DEFA);
    JGP_GET_INT(JGP_GET_OBJ_MINMAX_NDEFA);
    JGP_GET_INT(JGP_GET_OBJ_MINMAX_DEFA);
    JGP_GET_FLO(JGP_GET_OBJ_NDEFA);
    JGP_GET_FLO(JGP_GET_OBJ_DEFA);

private:
    jg_obj_get_t * _obj{};
    char * * _keys{};
    char * _k{};
};

#undef _JGP_ARGS
#undef _JGP_ARGS_MINMAX_NDEFA
#undef _JGP_ARGS_MINMAX_DEFA
#undef _JGP_ARGS_MINMAX
#undef _JGP_OPT
#undef _JGP_OPT_MINMAX_NDEFA
#undef _JGP_OPT_MINMAX_DEFA
#undef _JGP_OPT_MINMAX
#undef JGP_GET
#undef _JGP_GET_ROOT
#undef _JGP_GET_ARR
#undef _JGP_GET_OBJ
#undef _JGP_GET_OBJ_NDEFA
#undef _JGP_GET_OBJ_DEFA
#undef _JGP_GET_ROOT_MINMAX
#undef _JGP_GET_ARR_MINMAX
#undef _JGP_GET_OBJ_MINMAX_NDEFA
#undef _JGP_GET_OBJ_MINMAX_DEFA
#undef JGP_GET_JSON_TYPE
#undef JGP_GET_BOOL
#undef JGP_GET_INT
#undef JGP_GET_FLO

/*class ArrSet : Base {
public:
    inline ArrSet(
        std::shared_ptr<jg::_Session const> s,
        jg_arr_set_t * arr
    ) noexcept : Base(s) {
        _arr = arr;
    }
private:
    jg_arr_set_t * _arr{};
};

class ObjSet : Base {
public:
    inline ObjSet(
        std::shared_ptr<jg::_Session const> s,
        jg_obj_set_t * obj
    ) noexcept : Base(s) {
        _obj = obj;
    }
private:
    jg_obj_set_t * _obj{};
};*/

template <typename Functor>
inline auto Base::get_arr(
    Functor functor,
    size_t min_c,
    size_t max_c,
    std::string const & min_c_reason,
    std::string const & max_c_reason
) const {
    jg_opt_arr opt{
        min_c_reason.empty() ? nullptr : min_c_reason.c_str(),
        max_c_reason.empty() ? nullptr : max_c_reason.c_str(),
        min_c,
        max_c
    };
    jg_arr_get_t * arr{};
    size_t elem_c{};
    guard(functor(&opt, &arr, &elem_c));
    return ArrGet(_s, arr, elem_c);
}

template <typename Functor>
inline auto Base::get_obj(
    Functor functor,
    size_t min_c,
    size_t max_c,
    std::string const & min_c_reason,
    std::string const & max_c_reason
) const {
    char * * keys{};
    size_t key_c{};
    jg_opt_obj opt{
        &keys,
        &key_c,
        min_c_reason.empty() ? nullptr : min_c_reason.c_str(),
        max_c_reason.empty() ? nullptr : max_c_reason.c_str(),
        min_c,
        max_c,
    };
    jg_obj_get_t * obj{};
    guard(functor(&opt, &obj));
    return ObjGet(_s, obj, keys, key_c);
}

} // End of namespace jg

#endif
