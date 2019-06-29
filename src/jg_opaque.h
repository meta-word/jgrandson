// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include "jgrandson.h"

struct jg_opaque { // Usually 16 + (8 + 8) + (8 + (4 + (2 + 2))) = 48 bytes
    struct jg_val root_val;
    char * json_str;
    char * err_str;
    size_t json_str_i;
    int errnum;
    uint16_t ret; // jg_ret as a uint16_t
    uint16_t err_str_is_on_heap; // boolean
};
