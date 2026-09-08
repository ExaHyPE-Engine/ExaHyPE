// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TestParticle.h"

peano4::particles::assignmentchecks::tests::TestParticle::TestParticle(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  int                                          particleId,
  bool                                         isLocal
) :
  _x(x),
  _particleId(particleId),
  _isLocal(isLocal) {}

int peano4::particles::assignmentchecks::tests::TestParticle::getDepth() const { return _depth; }

int peano4::particles::assignmentchecks::tests::TestParticle::getPartid() const { return _particleId; }

tarch::la::Vector<DIMENSIONS, double> peano4::particles::assignmentchecks::tests::TestParticle::getVertexH() const {
  return _vertexH;
}

tarch::la::Vector<DIMENSIONS, double> peano4::particles::assignmentchecks::tests::TestParticle::getX() const {
  return _x;
}

bool peano4::particles::assignmentchecks::tests::TestParticle::isLocal() const { return _isLocal; }

peano4::particles::assignmentchecks::tests::TestParticle::ParallelState peano4::particles::assignmentchecks::tests::
  TestParticle::getParallelState() const {
  if (_isLocal) {
    return peano4::particles::assignmentchecks::tests::TestParticle::ParallelState::Local;
  } else {
    return peano4::particles::assignmentchecks::tests::TestParticle::ParallelState::Virtual;
  }
}

void peano4::particles::assignmentchecks::tests::TestParticle::setDepth(const int depth) { _depth = depth; }

void peano4::particles::assignmentchecks::tests::TestParticle::setPartid(const int particleId) {
  _particleId = particleId;
}

void peano4::particles::assignmentchecks::tests::TestParticle::setIsLocal(const bool isLocal) { _isLocal = isLocal; }

void peano4::particles::assignmentchecks::tests::TestParticle::setVertexH(
  const tarch::la::Vector<DIMENSIONS, double>& vertexH
) {
  _vertexH = vertexH;
}

void peano4::particles::assignmentchecks::tests::TestParticle::setX(const tarch::la::Vector<DIMENSIONS, double>& x) {
  _x = x;
}

void peano4::particles::assignmentchecks::tests::TestParticle::setParallelState(
  const peano4::particles::assignmentchecks::tests::TestParticle::ParallelState parallelState
) {
  if (parallelState == peano4::particles::assignmentchecks::tests::TestParticle::ParallelState::Local) {
    _isLocal = true;
  } else {
    _isLocal = false;
  }
}
