// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include "jgrandson.h"

struct jg_opaque { // 32 bytes total in most cases
    struct jg_val root_val;
    char * json_str;
    uint32_t json_str_i; // index to json_str to get context for parsing errors
    uint32_t ret; // jg_ret as a uint32_t
};
