// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Atomics.h"

namespace {
  // Taking these addresses makes the core library compile every atomic operation.
  [[maybe_unused]] void instantiateAtomicFunctions() {
    [[maybe_unused]] auto* load    = &tarch::accelerator::atomicLoad;
    [[maybe_unused]] auto* store   = &tarch::accelerator::atomicStore;
    [[maybe_unused]] auto* maximum = &tarch::accelerator::atomicMaximum;
  }
} // namespace
