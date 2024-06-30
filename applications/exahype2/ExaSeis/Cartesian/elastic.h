#pragma once

#include "Abstractelastic.h"
#include "tarch/logging/Log.h"

namespace applications {namespace exahype2 {namespace elastic {

  class elastic;

}}}


class applications::exahype2::elastic::elastic: public Abstractelastic {
  private:
    static tarch::logging::Log   _log;

  public:
    double* QuadraturePoints1d = kernels::elastic::Quadrature<double>::nodes;

    void pointSource(const float* const Q, const double* const x, const double t, const double dt, float* const forceVector, int n);
    void pointSource(const double* const Q, const double* const x, const double t, const double dt, double* const forceVector, int n);

    void initPointSourceLocations(double sourceLocation[NumberOfPointSources][Dimensions]) override;

};

