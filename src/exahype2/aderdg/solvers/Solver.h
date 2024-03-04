#pragma once

#include "tarch/la/Vector.h"
#include "exahype2/Solver.h"

namespace exahype2 {
  namespace solvers{
	  namespace aderdg{
	    class Solver;
	  }
  }
}

class exahype2::solvers::aderdg::Solver: public ::exahype2::Solver{
public:

  virtual tarch::la::Vector<Dimensions,double> getDomainSize() const = 0;
  
  virtual tarch::la::Vector<Dimensions,double> getDomainOffset() const = 0;
    
  virtual double getCoarsestMeshSize() const = 0;
  
  virtual int getCoarsestMeshLevel() const = 0;
  
  virtual int getMaximumAdaptiveMeshLevel() const = 0;
  
  virtual int getMaximumAdaptiveMeshDepth() const = 0;

};