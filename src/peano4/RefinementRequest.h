// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  struct RefinementRequest;
} // namespace peano4

/** Asks the spacetree to refine or to erase the cell at a level and lattice position. */
struct peano4::RefinementRequest {
public:
  enum class RefinementControl : std::uint8_t { Refine = 0, Erase = 1 };

  std::uint8_t refinementControl = static_cast<std::uint8_t>(RefinementControl::Refine);

  std::int32_t level = 0;

  tarch::la::Vector<DIMENSIONS, int> latticeIndex{0};

  RefinementRequest() noexcept = default;

  [[nodiscard]] INLINE_METHOD RefinementControl getRefinementControl() const noexcept;
  INLINE_METHOD void setRefinementControl(RefinementControl value) noexcept;

  [[nodiscard]] std::string toString() const;

  bool operator<(const RefinementRequest& other) const noexcept;
  bool operator==(const RefinementRequest& other) const noexcept;
};

std::ostream& operator<<(std::ostream& out, const peano4::RefinementRequest& request);

#include "peano4/RefinementRequest.cpph"
