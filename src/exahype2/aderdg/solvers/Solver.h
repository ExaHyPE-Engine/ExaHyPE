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
/*
  virtual tarch::la::Vector<Dimensions,double> getDomainSize() = 0;
  
  virtual tarch::la::Vector<Dimensions,double> getDomainOffset() = 0;
    
  virtual double getCoarsestMeshSize() = 0;
  
  virtual int getCoarsestMeshLevel() = 0;
  
  virtual int getMaximumAdaptiveMeshLevel() = 0;
  
  virtual int getMaximumAdaptiveMeshDepth() = 0;
*/
  
};