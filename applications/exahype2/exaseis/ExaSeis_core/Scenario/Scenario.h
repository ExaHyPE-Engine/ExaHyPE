#ifndef EXASEIS_SCENARIO_HEADER
#define EXASEIS_SCENARIO_HEADER

#include "../Context/DomainInformation.h"
#include "../Refinement/refinement.h"

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
    std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria
  ) {
    // no criteria
    // std::cout << "ScenarioCriteria" << std::endl;
  }

protected:
  DomainInformation* info;
};

#endif
