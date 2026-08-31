// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace toolbox::particles::assignmentchecks::tests {
  class TestParticle;
} // namespace toolbox::particles::assignmentchecks::tests

class toolbox::particles::assignmentchecks::tests::TestParticle final {
public:
  enum class ParallelState : int { Local = 0, Virtual = 1 };

  TestParticle(const tarch::la::Vector<DIMENSIONS, double>& x, int particleId, bool isLocal = false);

  int                                   getDepth() const;
  int                                   getPartid() const;
  tarch::la::Vector<DIMENSIONS, double> getVertexH() const;
  tarch::la::Vector<DIMENSIONS, double> getX() const;
  bool                                  isLocal() const;
  TestParticle::ParallelState           getParallelState() const;

  void setDepth(const int depth);
  void setPartid(const int particleId);
  void setIsLocal(const bool isLocal);
  void setVertexH(const tarch::la::Vector<DIMENSIONS, double>& vertexH);
  void setX(const tarch::la::Vector<DIMENSIONS, double>& x);
  void setParallelState(const TestParticle::ParallelState parallelState);

private:
  tarch::la::Vector<DIMENSIONS, double> _x{};
  int                                   _particleId{0};
  bool                                  _isLocal{false};

  // Size and depth of the vertex that holds the particle.
  tarch::la::Vector<DIMENSIONS, double> _vertexH{};
  int                                   _depth{0};
};
