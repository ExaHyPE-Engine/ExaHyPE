// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GaussLegendreBasis.h"

#include "GaussLegendreBasis.csnippet"
#include "peano4/utils/Loop.h"

double exahype2::aderdg::basis::legendre::interpolate(
  [[maybe_unused]] const double* offsetOfPatch,
  [[maybe_unused]] const double* sizeOfPatch,
  [[maybe_unused]] const double* x,
  [[maybe_unused]] int           numberOfUnknowns,
  [[maybe_unused]] int           unknown,
  [[maybe_unused]] int           order,
  [[maybe_unused]] const double* u
) {
  double result = 0.0;

  [[maybe_unused]] double xRef[Dimensions];
  xRef[0] = (x[0] - offsetOfPatch[0]) / sizeOfPatch[0];
  xRef[1] = (x[1] - offsetOfPatch[1]) / sizeOfPatch[1];
#if Dimensions == 3
  xRef[2] = (x[2] - offsetOfPatch[2]) / sizeOfPatch[2];
#endif

  // The code below evaluates the basis functions at the reference coordinates
  // and multiplies them with their respective coefficient.
  /*
  dfor(ii,order+1) { // Gauss-Legendre node indices
    int iGauss = peano4::utils::dLinearisedWithoutLookup(ii,order + 1);
    result += kernels::legendre::basisFunction[order][ii(0)](xRef[0]) *
              kernels::legendre::basisFunction[order][ii(1)](xRef[1]) *
              #if Dimensions==3
              kernels::legendre::basisFunction[order][ii(2)](xRef[2]) *
              #endif
               u[iGauss * numberOfUnknowns + unknown];
    assertion6(std::isfinite(result), result, unknown, iGauss, numberOfUnknowns, offsetOfPatch[0], sizeOfPatch[0]);
  }
  */

  return result;
}
