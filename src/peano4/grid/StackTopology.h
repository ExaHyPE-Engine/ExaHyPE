// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"

namespace peano4::grid {
  struct StackTopology;
} // namespace peano4::grid

struct peano4::grid::StackTopology {
  enum class BaseStack : int { Call = 0, Input = 1, Output = 2, NumberOfStacks };

  enum class FaceExchangeStack : int {
    Face0 = 0,
    Face1 = 1,
    Face2 = 2,
    Face3 = 3,
#if DIMENSIONS >= 3
    Face4 = 4,
    Face5 = 5,
#endif
    NumberOfStacks
  };

  enum class PeriodicBoundaryExchangeStack : int { Output = 0, Input = 1, NumberOfStacks };

  static constexpr int toIndex(StackTopology::PeriodicBoundaryExchangeStack s) { return static_cast<int>(s); }

  enum class ParallelExchangeStack : int {
    HorizontalOutput = 0,
    HorizontalInput  = 1,
    VerticalOutput   = 2,
    VerticalInput    = 3,
    NumberOfStacks
  };

  static constexpr int toIndex(StackTopology::ParallelExchangeStack s) { return static_cast<int>(s); }

  static constexpr int BaseStackOffset         = 0;
  static constexpr int FaceExchangeStackOffset = BaseStackOffset + static_cast<int>(BaseStack::NumberOfStacks);

  static constexpr int PeriodicBoundaryExchangeStackOffset = FaceExchangeStackOffset
                                                             + static_cast<int>(FaceExchangeStack::NumberOfStacks);
  static constexpr int TotalNumberOfPeriodicBoundaryExchangeStacks
    = static_cast<int>(PeriodicBoundaryExchangeStack::NumberOfStacks) * (THREE_POWER_D - 1);

  static constexpr int TotalNumberOfLocalExchangeStacks = PeriodicBoundaryExchangeStackOffset
                                                          + TotalNumberOfPeriodicBoundaryExchangeStacks;

  static constexpr int ParallelExchangeStackOffset = TotalNumberOfLocalExchangeStacks;

  static bool isInOutStack(int stackNumber);
  static bool isFaceExchangeStack(int stackNumber);
  static bool isLocalExchangeStack(int stackNumber);
  static bool isPeriodicBoundaryExchangeOutputStackNumber(int stackNumber);

  using PeriodicBoundaryStackIdentifier = std::pair<int, int>;
  static int                                       getOutputStackForPeriodicBoundaryExchange(int faceNumber);
  static std::set<PeriodicBoundaryStackIdentifier> getOutputStacksForPeriodicBoundaryExchange(
    const std::array<int, TWO_POWER_D>& flags
  );
  static int getPeriodicBoundaryExchangeInputStackNumberForOutputStack(int outputStackNumber);
  static PeriodicBoundaryStackIdentifier getPeriodicBoundaryExchangeInputStackNumberForOutputStack(
    PeriodicBoundaryStackIdentifier outputStackIdentifier
  );
  static int mapPeriodicBoundaryExchangeOutputStackOntoInputStack(int outputStackNumber);

  /**
   * Analyse to which of the 2d faces it is adjacent
   *
   * We enumerate again in the way we always enumerate, i.e. first the face
   * with the x axis as normal, then the one with the y axis, ... And after
   * these d faces, we enumerate all the opposite faces on the cube.
   *
   * The routine is non-trivial: We store the @f$ 2^d @f$ adjacency flags
   * per vertex, and now have to work backwards which periodic faces they
   * do represent.
   */
  static std::bitset<2 * DIMENSIONS> getPeriodicBoundaryNumber(const std::array<int, TWO_POWER_D>& flags);
};
