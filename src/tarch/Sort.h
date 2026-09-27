// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace tarch {
  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void sortAscending(Value* values, int size);
} // namespace tarch

#include "tarch/Sort.cpph"
