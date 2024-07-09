// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

// TODO: put back in
/* #include "peano4/utils/Globals.h" */

// TODO: remove again
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
  tarch::la::Vector<Dimensions, double> _x;
  int                                   _partid;
  bool                                  _isLocal;

  // helper variables of vertex particle is associated with
  tarch::la::Vector<Dimensions, double> _vertexH;
  int                                   _depth;

public:
  TestParticle(
    const tarch::la::Vector<Dimensions, double>& x,
    int                                          particleID,
    bool                                         local = false
  );

  int                                   getDepth() const;
  int                                   getPartid() const;
  tarch::la::Vector<Dimensions, double> getVertexH() const;
  tarch::la::Vector<Dimensions, double> getX() const;
  bool                                  isLocal() const;

  void setDepth(const int depth);
  void setIsLocal(const bool isLocal);
  void setVertexH(const tarch::la::Vector<Dimensions, double>& x);
  void setX(const tarch::la::Vector<Dimensions, double>& x);
};
