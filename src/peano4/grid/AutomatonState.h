// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct AutomatonState;
} // namespace peano4::grid

struct peano4::grid::AutomatonState {
  using FlagType = std::conditional_t<(DIMENSIONS <= 31), std::uint32_t, std::uint64_t>;

  int level{0};

  tarch::la::Vector<DIMENSIONS, double> x{};
  tarch::la::Vector<DIMENSIONS, double> h{};

  // Bits [0..DIMENSIONS-1] hold the even flags.
  FlagType flags{0};

  AutomatonState() noexcept = default;

  static constexpr FlagType evenMaskAll() { return (FlagType(1) << DIMENSIONS) - FlagType(1); }
  static constexpr FlagType evenMask(int i) { return FlagType(1) << i; }

  INLINE_METHOD bool getEvenFlag(int d) const noexcept { return (flags & evenMask(d)) != 0; }
  INLINE_METHOD void setEvenFlag(int d, bool value) noexcept { setFlag(evenMask(d), value); }
  INLINE_METHOD void flipEvenFlag(int d) noexcept { flags ^= evenMask(d); }

  INLINE_METHOD bool getFlag(FlagType bit) const noexcept { return (flags & bit) != 0; }
  INLINE_METHOD void setFlag(FlagType bit) noexcept { flags |= bit; }
  INLINE_METHOD void clearFlag(FlagType bit) noexcept { flags &= ~bit; }
  INLINE_METHOD void setFlag(FlagType bit, bool value) noexcept { value ? setFlag(bit) : clearFlag(bit); }

  std::string toString() const;

  static void refineState(
    const AutomatonState&              coarseGrid,
    AutomatonState                     fineGrid[THREE_POWER_D],
    tarch::la::Vector<DIMENSIONS, int> fineGridPosition = tarch::la::Vector<DIMENSIONS, int>(0),
    int                                axis             = DIMENSIONS - 1
  );
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::AutomatonState& state);
