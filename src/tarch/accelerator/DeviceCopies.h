// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::accelerator {
  template <typename WriteDeviceCopy, typename Value>
  INLINE_METHOD bool addDeviceCopy(const Value& hostValue);
  bool addDeviceCopyUpload(std::function<void()> upload);
  void uploadDeviceCopies();
} // namespace tarch::accelerator

#include "tarch/accelerator/DeviceCopies.cpph"
