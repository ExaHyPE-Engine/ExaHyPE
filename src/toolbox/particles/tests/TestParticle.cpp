#include "TestParticle.h"

toolbox::particles::tests::TestParticle::TestParticle(const tarch::la::Vector<Dimensions, double>& x, double h):
  _x(x),
  _h(h) {}

tarch::la::Vector<Dimensions, double> toolbox::particles::tests::TestParticle::getX() const { return _x; }

double toolbox::particles::tests::TestParticle::getSearchRadius() const { return _h; }
