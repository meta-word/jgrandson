// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include "jgrandson.h"

struct jg_opaque { // Usually 16 + (8 + 8) + (8 + (4 + (2 + (1 + 1))) = 48 bytes
    struct jg_val root_val;
    uint8_t * json_str;
    char * err_str;
    size_t json_str_i;
    int errnum;
    uint16_t ret; // jg_ret as a uint16_t
    uint8_t json_str_needs_free; // boolean
    uint8_t err_str_needs_free; // boolean
};

void free_json_str(
    jg_t * jg
);
