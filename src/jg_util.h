// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include "jgrandson.h"

struct jg_opaque {
    struct jg_val root_val;
    uint8_t const * json_str; // the start of the JSON string to be parsed
    uint8_t const * json_cur; // current parsing position within json_str
    uint8_t const * json_over; // the byte following the end of the json_str buf
    char const * err_str;
    bool json_str_needs_free;
    bool err_str_needs_free;
    int errnum;
    jg_ret ret;
};

void free_json_str(
    jg_t * jg
);
