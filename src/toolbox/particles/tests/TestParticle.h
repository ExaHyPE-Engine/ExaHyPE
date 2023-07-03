// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/utils/Globals.h"

// #include <concepts>

namespace toolbox {
  namespace particles {
    namespace tests {
      class TestParticle;
    } // namespace tests
  }   // namespace particles
} // namespace toolbox

class toolbox::particles::tests::TestParticle {
public:
  TestParticle(const tarch::la::Vector<Dimensions, double>& x, double h);

  tarch::la::Vector<Dimensions, double> getX() const;
  double                                getSearchRadius() const;

  tarch::la::Vector<Dimensions, double> _x;
  double                                _h;
};
