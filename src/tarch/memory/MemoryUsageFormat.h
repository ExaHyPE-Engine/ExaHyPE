// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::memory {
  enum class MemoryUsageFormat { MByte, GByte };
} // namespace tarch::memory

namespace tarch {
  using MemoryUsageFormat = tarch::memory::MemoryUsageFormat; // TODO: Remove
} // namespace tarch
