// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#pragma once

#include "exahype2/Solver.h"
#include "tarch/la/Vector.h"

namespace exahype2::solvers::aderdg {
  class Solver;
} // namespace exahype2::solvers::aderdg

class exahype2::solvers::aderdg::Solver: public exahype2::Solver {
public:
  virtual tarch::la::Vector<Dimensions, double> getDomainSize() const = 0;

  virtual tarch::la::Vector<Dimensions, double> getDomainOffset() const = 0;

  virtual double getCoarsestMeshSize() const = 0;

  virtual int getCoarsestMeshLevel() const = 0;

  virtual int getMaximumAdaptiveMeshLevel() const = 0;

  virtual int getMaximumAdaptiveMeshDepth() const = 0;
};
