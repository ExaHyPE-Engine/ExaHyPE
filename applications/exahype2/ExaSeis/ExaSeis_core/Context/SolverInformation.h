#ifndef EXASEIS_SOLVERINFORMATION_HEADER
#define EXASEIS_SOLVERINFORMATION_HEADER

// #include "exahype/solvers/Solver.h"
#include "exahype2/aderdg/solvers/Solver.h"

class SolverInformation {
public:
  SolverInformation(exahype2::solvers::aderdg::Solver* a_solver){};
  virtual bool isDG() = 0;
};
#endif
