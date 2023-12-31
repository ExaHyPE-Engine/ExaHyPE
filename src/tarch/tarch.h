// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "memory/memory.h"

/**
 * @namespace tarch
 *
 * tarch denotes the technical architecture layer. It is used within Peano, but it
 * is designed in a way that it is more or less independent of Peano, i.e.,
 * Peano uses the tarch but not the other way round.
 *
 * \section Debug level
 *
 * - PeanoDebug = 1: Enable tracing
 * - PeanoDebug = 2: Enable assertions (includes level 1)
 * - PeanoDebug = 4: Enable debug messages (includes level 2)
 *
 * \section Default libraries
 *
 * By default, Peano 4 builds all core libraries in different versions:
 *
 * - A debug variant which holds a compile with PeanoDebug=4
 * - A trace variant which holds a compile with PeanoDebug=1
 * - A production variant with PeanoDebug=0
 *
 * Note: The trace variant does not contain any assertions, as assertions are
 * typically expensive and slow down the code considerably.
 *
 */
namespace tarch {}

/**
 * This is the marker that is to be used after the argument list of a
 * function declaration.
 */
#if defined(UseManualInlining)
#define InlineMethod __attribute__((always_inline))
#else
#define InlineMethod
#endif
