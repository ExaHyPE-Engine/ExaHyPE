#ifndef EXASEIS_SCENARIO_HEADER
#define EXASEIS_SCENARIO_HEADER

#include "exahype2/aderdg/kernels/Utils/KernelUtils.h"
// #include "kernels/GaussLegendreBasis.h"

// #include "exahype/solvers/Solver.h"
#include "../Context/DomainInformation.h"
#include "../Refinement/refinement.h"
#include "exahype2/aderdg/solvers/Solver.h"

template <class Shortcuts, int basisSize>
class Scenario {

public:
  Scenario(DomainInformation* a_info) { info = a_info; };

  virtual void initUnknownsPointwise(
    const double* const                          x,
    const tarch::la::Vector<Dimensions, double>& center,
    const double                                 t,
    const double                                 dt,
    double*                                      Q
  ) = 0;

  virtual void initPointSourceLocation(double pointSourceLocation[][3]){};

  virtual void setPointSourceVector(
    const double* const Q, const double* const x, const double t, const double dt, double* forceVector, int n
  ){};

  virtual void refinementCriteria(
    exahype2::solvers::aderdg::Solver* solver, std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria
  ) {
    // no criteria
    // std::cout << "ScenarioCriteria" << std::endl;
  }

protected:
  DomainInformation* info;
};

#endif
