// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch.h"

#include <climits>
#include <iostream>
#include <limits>
#include <stddef.h>

static_assert(sizeof(void*) == 8, "Assuming 64-bit build; i.e., 8-byte pointers");
static_assert(std::numeric_limits<float>::is_iec559, "std::numeric_limits<float>::is_iec559");
static_assert(std::numeric_limits<double>::is_iec559, "std::numeric_limits<double>::is_iec55");
static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
static_assert(sizeof(char) == 1, "sizeof(short) != 1");
static_assert(sizeof(short) == 2, "sizeof(short) != 2");
static_assert(sizeof(int) == 4, "sizeof(int) != 4");
static_assert(sizeof(long long) == 8, "sizeof(long long) != 8");
static_assert('A' == 65, "A != 65");
