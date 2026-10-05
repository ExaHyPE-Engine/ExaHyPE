// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace tarch::accelerator {
  template <typename WriteDeviceCopy, typename Value>
  INLINE_METHOD bool addDeviceCopy(const Value& hostValue);
  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value& getDeviceCopy();
  bool addDeviceCopyUpload(std::function<void()> upload);
  void uploadDeviceCopies();
} // namespace tarch::accelerator

#include "tarch/accelerator/DeviceCopies.cpph"
