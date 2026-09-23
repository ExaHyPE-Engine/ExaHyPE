// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/CellMarker.h"

namespace peano4::particles::tests {
  class TestParticle;
} // namespace peano4::particles::tests

/** Stands in for a generated particle in the tests of the particle helpers. */
class peano4::particles::tests::TestParticle final {
public:
  TestParticle(const tarch::la::Vector<DIMENSIONS, double>& x, double h);

  [[nodiscard]] tarch::la::Vector<DIMENSIONS, double> getX() const;
  [[nodiscard]] double                                getSearchRadius() const;

private:
  tarch::la::Vector<DIMENSIONS, double> _x{};
  double                                _h{0.0};
};
