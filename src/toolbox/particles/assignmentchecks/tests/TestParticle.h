// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/utils/Globals.h"

// #include <concepts>

namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace tests {
        class TestParticle;
      } // namespace tests
    } // assignmentchecks
  }   // namespace particles
} // namespace toolbox

class toolbox::particles::assignmentchecks::tests::TestParticle {
public:
  TestParticle(const tarch::la::Vector<Dimensions, double>& x, int particleID);

  tarch::la::Vector<Dimensions, double> getX() const;
  int                                getPartid() const;

  void setX(const tarch::la::Vector<Dimensions, double>& x);

  tarch::la::Vector<Dimensions, double> _x;
  int                                   _partid;
};
