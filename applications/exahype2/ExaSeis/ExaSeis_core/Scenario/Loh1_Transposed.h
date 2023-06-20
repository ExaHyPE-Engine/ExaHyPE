#ifndef EXASEIS_SCENARIOLOHONE_TRANSPOSED_HEADER
#define EXASEIS_SCENARIOLOHONE_TRANSPOSED_HEADER
#include "Scenario.h"

template <class Shortcuts, int basisSize>
class Loh1_Transposed: public Scenario<Shortcuts, basisSize> {
private:
  double m_pointSourceLocation[3];

public:
  Loh1_Transposed(DomainInformation* info):
    Scenario<Shortcuts, basisSize>(info) {
    // place point source in center of topography 2km below surface
    m_pointSourceLocation[0] = 2.0;
    m_pointSourceLocation[1] = 0.0;
    m_pointSourceLocation[2] = 0.0;
  };

  void initUnknownsPointwise(
    const double* const                          x,
    const tarch::la::Vector<Dimensions, double>& center,
    const double                                 t,
    const double                                 dt,
    double*                                      Q
  ) {
    Shortcuts s;

#ifdef ISOTROPIC
    if (x[0] <= (1.0 + 1.0e-6) && center[0] < 1.0) {
      Q[s.rho] = 2.6;
      Q[s.cp]  = 4.0;
      Q[s.cs]  = 2.0;
    } else if (x[0] >= (1.0 - 1.0e-6) && center[0] > 1.0) {
      Q[s.rho] = 2.7;
      Q[s.cp]  = 6.0;
      //      Q[s.cs ] = 3.343;
      Q[s.cs] = 3.464;
    } else {
      assertion3(false, "Point not initialised", x[0], center[0]);
    }
#elif defined ANISOTROPIC
    assertion1(false, "Loh1_transposed Not implemented in the Anisotropic case");
#else
#error Whole space problem only defined for Isotropic and Anisotropic Scenario
#endif // ISOTROPIC
  }

  void initPointSourceLocation(double pointSourceLocation[][3]) {
    pointSourceLocation[0][0] = m_pointSourceLocation[0];
    pointSourceLocation[0][1] = m_pointSourceLocation[1];
    pointSourceLocation[0][2] = m_pointSourceLocation[2];
  }

  void setPointSourceVector(
    const double* const Q, const double* const x, const double t, const double dt, double* forceVector, int n
  ) {

    assertion2(n == 0, "Only a single pointSource for Loh1_transposed", n);

    constexpr double t0 = 0.1;
    constexpr double M0 = 1000.0;

    double f = M0 * t / (t0 * t0) * std::exp(-t / t0);

    Shortcuts s;
    forceVector[s.sigma + 5] = f;
  }

  void refinementCriteria(
    exahype2::solvers::aderdg::Solver* solver, std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria
  ) override {
    Shortcuts s;

    criteria.push_back(new Refinement::StaticAMR<Shortcuts>);
    criteria.push_back(new Refinement::CoarseBoundaryLayer<Shortcuts>(
      solver->getMaximumAdaptiveMeshLevel(),
      solver->getCoarsestMeshLevel(),
      solver->getCoarsestMeshSize(),
      0,
      &(solver->getDomainSize()[0]),
      &(solver->getDomainOffset()[0])
    ));
#ifdef _CUSTOM_COORDINATES
    criteria.push_back(new Refinement::RefineDownToPositionCustomCoordinates<Shortcuts>(
      basisSize,
      solver->getMaximumAdaptiveMeshLevel(),
      m_pointSourceLocation,
      0,
      static_cast<exahype::solvers::ADERDGSolver*>(solver)->FCoeff[basisSize - 1][0]
    ));

#else
    double left[3];
    double right[3];

    left[0] = -0.0;
    left[1] = -3.0;
    left[2] = -3.0;

    right[0] = 3.0;
    right[1] = 12.0;
    right[2] = 12.0;

    criteria.push_back(
      new Refinement::RefineBetweenPositions<Shortcuts>(solver->getMaximumAdaptiveMeshLevel(), left, right)
    );
#endif
  };
};
#endif
