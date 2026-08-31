// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"

namespace peano4::grid {
  struct StackTopology;
} // namespace peano4::grid

struct peano4::grid::StackTopology {
  enum class BaseStack : int { Call = 0, Input = 1, NumberOfStacks };

  static constexpr int TotalNumberOfLocalExchangeStacks = static_cast<int>(BaseStack::NumberOfStacks);

  static bool isInOutStack(const int stackNumber);
};
