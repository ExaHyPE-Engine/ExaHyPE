#include "TestParticle.h"

toolbox::particles::assignmentchecks::tests::TestParticle::TestParticle(const tarch::la::Vector<Dimensions, double>& x, int particleID):
  _x(x),
  _partid(particleID) {}

tarch::la::Vector<Dimensions, double> toolbox::particles::assignmentchecks::tests::TestParticle::getX() const { return _x; }

void toolbox::particles::assignmentchecks::tests::TestParticle::setX(const tarch::la::Vector<Dimensions, double>& x){
  _x = x;
}

int toolbox::particles::assignmentchecks::tests::TestParticle::getPartid() const { return _partid; }
