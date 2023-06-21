#ifndef EXASEIS_SOLVERFV_INFORMATION_HEADER
#define EXASEIS_SOLVERFV_INFORMATION_HEADER

#include <algorithm>
#include "exahype/solvers/FiniteVolumesSolver.h"
#include "SolverInformation.h"

template<int patchSize>
class SolverInformationFV : public SolverInformation{
public:
 SolverInformationFV(exahype::solvers::FiniteVolumesSolver* a_solver): SolverInformation(a_solver){
  };
  
  bool isDG() override{
    return false;
  }
};
#endif
