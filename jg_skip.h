// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include "jgrandson.h"

void skip_any_whitespace(
    uint8_t const * * u,
    uint8_t const * const u_over
);

char const * skip_string(
    uint8_t const * * u,
    uint8_t const * const u_over
);

char const * skip_array(
    uint8_t const * * u,
    uint8_t const * const u_over
);

char const * skip_object(
    uint8_t const * * u,
    uint8_t const * const u_over
);

void skip_element(
    uint8_t const * * u
);
