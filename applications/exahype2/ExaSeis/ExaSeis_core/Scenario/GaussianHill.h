#ifndef EXASEIS_GAUSSIANHILL_HEADER
#define EXASEIS_GAUSSIANHILL_HEADER

#include "Scenario.h"

template <class Variables, int basisSize>
class GaussianHill: public Scenario<Variables, basisSize> {

private:
  double m_pointSourceLocation[3];

public:
  GaussianHill(DomainInformation* info):
    Scenario<Variables, basisSize>(info) {
    m_pointSourceLocation[0] = -3.0;
    m_pointSourceLocation[1] = 3.0;
    m_pointSourceLocation[2] = 0.0;
  };

  virtual void initUnknownsPointwise(
    const double* const                          x,
    const tarch::la::Vector<Dimensions, double>& center,
    const double                                 t,
    const double                                 dt,
    double*                                      Q
  ) {
    Variables s;
    Q[s.rho] = 2.6;
    Q[s.cp]  = 6.0;
    Q[s.cs]  = 3.464;
  }

  virtual void initPointSourceLocation(double pointSourceLocation[][3]) {
    pointSourceLocation[0][0] = m_pointSourceLocation[0];
    pointSourceLocation[0][1] = m_pointSourceLocation[1];
    pointSourceLocation[0][2] = m_pointSourceLocation[2];
  }

  virtual void setPointSourceVector(
    const double* const Q, const double* const x, const double t, const double dt, double* forceVector, int n
  ) {

    constexpr double pi = 3.14159265359;
    constexpr double t0 = 0.1;
    constexpr double M0 = 1000.0;

    double f = M0 * t / (t0 * t0) * std::exp(-t / t0);

    Variables s;
    forceVector[s.sigma + 4] = f;
  };

  virtual void refinementCriteria(
    exahype2::solvers::aderdg::Solver* solver, std::vector<Refinement::RefinementCriterion<Variables>*>& criteria
  ) override {
    criteria.push_back(new Refinement::RefinePositionCustomCoordinates<Variables>(
      m_pointSourceLocation,
      basisSize,
      static_cast<exahype::solvers::ADERDGSolver*>(solver)->FCoeff[basisSize - 1][0],
      static_cast<exahype::solvers::ADERDGSolver*>(solver)->FCoeff[basisSize - 1][1]
    ));
  };
};

#endif
