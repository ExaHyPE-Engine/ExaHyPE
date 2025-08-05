// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace tests {
        class TestParticle;
      } // namespace tests
    }   // namespace assignmentchecks
  }     // namespace particles
} // namespace toolbox

class toolbox::particles::assignmentchecks::tests::TestParticle {
private:
  tarch::la::Vector<DIMENSIONS, double> _x;
  int                                   _partid;
  bool                                  _isLocal;

  // helper variables of vertex particle is associated with
  tarch::la::Vector<DIMENSIONS, double> _vertexH;
  int                                   _depth;

public:
  // Mimic proper particle behaviour
  enum class ParallelState : int { Local = 0, Virtual = 1 };

public:
  TestParticle(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          particleID,
    bool                                         local = false
  );

  int                                   getDepth() const;
  int                                   getPartid() const;
  tarch::la::Vector<DIMENSIONS, double> getVertexH() const;
  tarch::la::Vector<DIMENSIONS, double> getX() const;
  bool                                  isLocal() const;
  TestParticle::ParallelState           getParallelState() const;

  void setDepth(const int depth);
  void setPartid(const int partid);
  void setIsLocal(const bool isLocal);
  void setVertexH(const tarch::la::Vector<DIMENSIONS, double>& x);
  void setX(const tarch::la::Vector<DIMENSIONS, double>& x);
  void setParallelState(const TestParticle::ParallelState parallelState);
};
