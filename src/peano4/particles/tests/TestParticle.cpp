// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestParticle.h"

peano4::particles::tests::TestParticle::TestParticle(const tarch::la::Vector<DIMENSIONS, double>& x, double h) :
  _x(x),
  _h(h) {}

tarch::la::Vector<DIMENSIONS, double> peano4::particles::tests::TestParticle::getX() const { return _x; }

double peano4::particles::tests::TestParticle::getSearchRadius() const { return _h; }
