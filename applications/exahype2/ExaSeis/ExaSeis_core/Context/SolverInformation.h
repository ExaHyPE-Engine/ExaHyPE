#ifndef EXASEIS_SOLVERINFORMATION_HEADER
#define EXASEIS_SOLVERINFORMATION_HEADER

#include "exahype2/Solver.h"

class SolverInformation {
public:
  SolverInformation(exahype2::Solver* a_solver){};
  virtual bool isDG() = 0;
};
#endif
