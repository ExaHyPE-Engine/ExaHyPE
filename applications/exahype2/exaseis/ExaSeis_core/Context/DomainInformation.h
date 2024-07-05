#ifndef EXASEIS_DOMAIN_INFORMATION_HEADER
#define EXASEIS_DOMAIN_INFORMATION_HEADER

#include <algorithm>
// #include "exahype/solvers/Solver.h"
// #include "exahype2/aderdg/solvers/Solver.h"

class DomainInformation {
public:
  DomainInformation(
    int coarsestMeshLevel,
    double coarsestMeshSize, 
    double maxAdaptiveDepth, 
    tarch::la::Vector<Dimensions,double> _domainOffset, 
    tarch::la::Vector<Dimensions,double> _domainSize):

    dx(coarsestMeshSize),
    meshLevel(coarsestMeshLevel),
    maxAdaptiveDepth(maxAdaptiveDepth){

    domainOffset[0] = _domainOffset[0];
    domainOffset[1] = _domainOffset[1];
    domainOffset[2] = _domainOffset[2];

    domainSize[0] = _domainSize[0];
    domainSize[1] = _domainSize[1];
    domainSize[2] = _domainSize[2];

    max_dx = dx * std::pow(1 / 3.0, maxAdaptiveDepth);

    elements[0] = std::round(domainSize[0] / dx);
    elements[1] = std::round(domainSize[1] / dx);
    elements[2] = std::round(domainSize[2] / dx);

    max_elements[0] = std::round(domainSize[0] / max_dx);
    max_elements[1] = std::round(domainSize[1] / max_dx);
    max_elements[2] = std::round(domainSize[2] / max_dx);

    assertion1(std::isfinite(meshLevel), "Coarsest mesh level not finite");
    assertion1(
      std::isfinite(domainOffset[0]) && std::isfinite(domainOffset[1]) && std::isfinite(domainOffset[2]),
      "Domain Offset not finite"
    );

    assertion1(
      std::isfinite(domainSize[0]) && std::isfinite(domainSize[1]) && std::isfinite(domainSize[2]),
      "Domain Size not finite"
    );
  }

  void getCenter(double* center) {
    center[0] = domainOffset[0] + domainSize[0] * 0.5;
    center[1] = domainOffset[1] + domainSize[1] * 0.5;
    center[2] = domainOffset[2] + domainSize[2] * 0.5;
  }

  double getDx() { return dx; }

  const int    meshLevel;
  const int    maxAdaptiveDepth;
  const double dx;
  double       domainOffset[3];
  double       domainSize[3];
  double       elements[3];

  double max_dx;
  double max_elements[3];
};
#endif
