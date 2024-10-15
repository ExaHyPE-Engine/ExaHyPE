// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TestParticle.h"

toolbox::particles::assignmentchecks::tests::TestParticle::TestParticle(
  const tarch::la::Vector<Dimensions, double>& x,
  int                                          particleID,
  bool                                         isLocal
):
  _x(x),
  _partid(particleID),
  _isLocal(isLocal) {}


int toolbox::particles::assignmentchecks::tests::TestParticle::getDepth(
) const {
  return _depth;
}

int toolbox::particles::assignmentchecks::tests::TestParticle::getPartid(
) const {
  return _partid;
}

tarch::la::Vector<Dimensions, double> toolbox::particles::assignmentchecks::
  tests::TestParticle::getVertexH() const {
  return _vertexH;
}

tarch::la::Vector<Dimensions, double> toolbox::particles::assignmentchecks::
  tests::TestParticle::getX() const {
  return _x;
}

bool toolbox::particles::assignmentchecks::tests::TestParticle::isLocal(
) const {
  return _isLocal;
}

toolbox::particles::assignmentchecks::tests::TestParticle::ParallelState toolbox::
  particles::assignmentchecks::tests::TestParticle::getParallelState() const {
  if (_isLocal) {
    return toolbox::particles::assignmentchecks::tests::TestParticle::
      ParallelState::Local;
  } else {
    return toolbox::particles::assignmentchecks::tests::TestParticle::
      ParallelState::Virtual;
  }
}

void toolbox::particles::assignmentchecks::tests::TestParticle::setDepth(
  const int depth
) {
  _depth = depth;
}

void toolbox::particles::assignmentchecks::tests::TestParticle::setIsLocal(
  const bool isLocal
) {
  _isLocal = isLocal;
}


void toolbox::particles::assignmentchecks::tests::TestParticle::setVertexH(
  const tarch::la::Vector<Dimensions, double>& vertexH
) {
  _vertexH = vertexH;
}

void toolbox::particles::assignmentchecks::tests::TestParticle::setX(
  const tarch::la::Vector<Dimensions, double>& x
) {
  _x = x;
}

void toolbox::particles::assignmentchecks::tests::TestParticle::setParallelState(
  toolbox::particles::assignmentchecks::tests::TestParticle::ParallelState
    parallelState
) {
  if (parallelState == toolbox::particles::assignmentchecks::tests::TestParticle::ParallelState::Local) {
    _isLocal = true;
  } else {
    _isLocal = false;
  }
}
