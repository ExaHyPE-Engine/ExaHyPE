// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/dataview/CellMarker.h"

// #include <concepts>

namespace peano4::particles::tests {
  class TestParticle;
} // namespace peano4::particles::tests

class peano4::particles::tests::TestParticle {
public:
  TestParticle(const tarch::la::Vector<DIMENSIONS, double>& x, double h);

  tarch::la::Vector<DIMENSIONS, double> getX() const;
  double                                getSearchRadius() const;

  tarch::la::Vector<DIMENSIONS, double> _x;
  double                                _h;
};
